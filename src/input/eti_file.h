/*
 *    Copyright (C) 2026
 *
 *    This file is part of the welle.io.
 *
 *    AI-generated: CVirtualInput implementation for raw ETI-NI files and
 *    pipes (including stdin via "-").  Handles two on-disk formats:
 *      - Raw   : 6144-byte frames, sync word FF F8 C5 49 / FF 07 3A B6 at byte 0
 *      - Streamed : 2-byte LE frame-length prefix followed by the frame
 *    For non-seekable sources (pipes, /dev/stdin) the bytes consumed during
 *    format detection are saved in a replay buffer (peekedBytes).
 */

#ifndef ETI_FILE_H
#define ETI_FILE_H

#include <cstdio>
#include <string>
#include <vector>

#include "virtual_input.h"

class CETIFile : public CVirtualInput
{
public:
    CETIFile(RadioControllerInterface& radioController);
    ~CETIFile(void) override;

    void setFileName(const std::string& fileName);

    void setFrequency(int frequency) override;
    int getFrequency() const override;
    bool restart(void) override;
    bool is_ok() override;
    void stop(void) override;
    void reset() override;
    int32_t getSamples(DSPCOMPLEX* v, int32_t size) override;
    std::vector<DSPCOMPLEX> getSpectrumSamples(int size) override;
    int32_t getSamplesToRead(void) override;
    float getGain() const override;
    float setGain(int gain) override;
    int getGainCount() override;
    void setAgc(bool agc) override;
    std::string getDescription() override;
    CDeviceID getID() override;

    bool isEtiInput(void) const override;
    int32_t getEtiFrame(uint8_t* buffer, size_t size) override;

private:
    enum class StreamType {
        None,
        Raw,
        Streamed,
    };

    bool openFile();
    bool identifyStreamType();
    bool isRawSync(const uint8_t *buf) const;

    RadioControllerInterface& radioController;
    std::string fileName;
    FILE *fd = nullptr;
    bool ok = false;
    bool isStdin = false;
    StreamType streamType = StreamType::None;

    // Bytes from stream-type detection that must be replayed before further reads.
    // Uses a vector so it can hold the full scan tail (up to ~6144 bytes).
    std::vector<uint8_t> peekedBytes;
    size_t peekedPos = 0;

    size_t readBytes(uint8_t* buf, size_t n);
};

#endif
