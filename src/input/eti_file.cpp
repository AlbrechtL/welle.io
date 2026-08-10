/*
 *    Copyright (C) 2026
 *
 *    This file is part of the welle.io.
 *
 *    AI-generated: see eti_file.h for design notes.
 */

#include "eti_file.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <cerrno>

CETIFile::CETIFile(RadioControllerInterface& radioController) :
    radioController(radioController)
{
}

CETIFile::~CETIFile(void)
{
    stop();
}

void CETIFile::setFileName(const std::string& fileName)
{
    this->fileName = fileName;
    (void)restart();
}

void CETIFile::setFrequency(int frequency)
{
    (void)frequency;
}

int CETIFile::getFrequency() const
{
    return 0;
}

bool CETIFile::openFile()
{
    stop();

    if (fileName.empty()) {
        ok = false;
        return false;
    }

    peekedBytes.clear();
    peekedPos = 0;

    if (fileName == "-") {
        // Read from stdin
        fd = stdin;
        isStdin = true;
    }
    else {
        isStdin = false;
        fd = fopen(fileName.c_str(), "rb");
        if (!fd) {
            ok = false;
            std::clog << "ETIFile: Cannot open file: " << fileName << std::endl;
            return false;
        }
    }

    if (!identifyStreamType()) {
        if (!isStdin) {
            fclose(fd);
        }
        fd = nullptr;
        ok = false;
        radioController.onMessage(message_level_t::Error,
                "Unknown ETI stream format");
        return false;
    }

    ok = true;
    return true;
}

bool CETIFile::identifyStreamType()
{
    // The upstream tool (dab2eti, eti-cmdline, ...) may already be mid-frame
    // when we first read from the pipe.  Scan forward up to one full ETI frame
    // length (6144 bytes) looking for a valid sync word, then treat everything
    // from that sync word onward as the beginning of the stream.
    const size_t kMaxScan = 6144 + 64; // a little past one frame
    std::vector<uint8_t> scan;
    scan.reserve(kMaxScan);

    // Read scan bytes in chunks.
    {
        uint8_t chunk[512];
        while (scan.size() < kMaxScan) {
            const size_t want = std::min(sizeof(chunk), kMaxScan - scan.size());
            const size_t got  = fread(chunk, 1, want, fd);
            if (got == 0) break;
            scan.insert(scan.end(), chunk, chunk + got);

            // Check for sync at any position seen so far.
            const size_t end = scan.size();
            for (size_t i = 0; i + 4 <= end; i++) {
                if (!isRawSync(scan.data() + i)) continue;

                // Found a sync word at offset i.
                streamType = StreamType::Raw;

                if (i > 0) {
                    std::clog << "ETIFile: sync found at byte offset " << i
                              << ", skipped " << i << " prefix bytes" << std::endl;
                }

                // For a seekable file seek to the sync position;
                // for a pipe store everything from the sync onward in the peek buffer.
                if (fseek(fd, static_cast<long>(i), SEEK_SET) != 0) {
                    // Not seekable — store tail bytes for replay.
                    peekedBytes.assign(scan.begin() + i, scan.end());
                    peekedPos = 0;
                }
                return true;
            }
        }
    }

    // No sync found — log the first bytes to help diagnose the format.
    std::clog << "ETIFile: no ETI sync found in " << scan.size() << " bytes. First bytes (hex):";
    for (size_t i = 0; i < std::min(scan.size(), (size_t)24); i++) {
        char buf[4];
        std::snprintf(buf, sizeof(buf), " %02x", scan[i]);
        std::clog << buf;
    }
    std::clog << std::endl;
    return false;
}

// Helper: read n bytes, draining the peek buffer first.
size_t CETIFile::readBytes(uint8_t* buf, size_t n)
{
    size_t filled = 0;

    // Drain peeked bytes first.
    if (peekedPos < peekedBytes.size()) {
        const size_t avail = peekedBytes.size() - peekedPos;
        const size_t take  = std::min(avail, n);
        std::memcpy(buf, peekedBytes.data() + peekedPos, take);
        peekedPos += take;
        filled    += take;
    }

    if (filled < n) {
        filled += fread(buf + filled, 1, n - filled, fd);
    }

    return filled;
}

bool CETIFile::isRawSync(const uint8_t *buf) const
{
    if (!buf || buf[0] != 0xFF) {
        return false;
    }

    const bool fsyncA = (buf[1] == 0xF8 && buf[2] == 0xC5 && buf[3] == 0x49);
    const bool fsyncB = (buf[1] == 0x07 && buf[2] == 0x3A && buf[3] == 0xB6);
    return fsyncA || fsyncB;
}

bool CETIFile::restart(void)
{
    return openFile();
}

bool CETIFile::is_ok()
{
    return ok;
}

void CETIFile::stop(void)
{
    if (fd && !isStdin) {
        fclose(fd);
    }
    fd = nullptr;
    ok = false;
}

void CETIFile::reset()
{
    if (!isStdin && fd) {
        fseek(fd, 0, SEEK_SET);
    }
}

int32_t CETIFile::getSamples(DSPCOMPLEX* v, int32_t size)
{
    (void)v;
    (void)size;
    return 0;
}

std::vector<DSPCOMPLEX> CETIFile::getSpectrumSamples(int size)
{
    return std::vector<DSPCOMPLEX>(static_cast<size_t>(std::max(0, size)), DSPCOMPLEX(0.0f, 0.0f));
}

int32_t CETIFile::getSamplesToRead(void)
{
    return 0;
}

float CETIFile::getGain() const
{
    return 0.0f;
}

float CETIFile::setGain(int gain)
{
    (void)gain;
    return 0.0f;
}

int CETIFile::getGainCount()
{
    return 0;
}

void CETIFile::setAgc(bool agc)
{
    (void)agc;
}

std::string CETIFile::getDescription()
{
    return isStdin ? "eti_file (stdin)" : "eti_file (" + fileName + ")";
}

CDeviceID CETIFile::getID()
{
    return CDeviceID::ETI_FILE;
}

bool CETIFile::isEtiInput(void) const
{
    return true;
}

int32_t CETIFile::getEtiFrame(uint8_t* buffer, size_t size)
{
    if (!fd || !buffer || size < 6144) {
        return -1;
    }

    memset(buffer, 0x55, size);

    if (streamType == StreamType::Raw) {
        const size_t got = readBytes(buffer, 6144);
        if (got == 0) {
            return 0;
        }
        if (got != 6144) {
            return -1;
        }
        return 6144;
    }

    if (streamType == StreamType::Streamed) {
        uint8_t lenBytes[2];
        if (readBytes(lenBytes, 2) != 2) {
            return 0;
        }

        const uint16_t frameSize = static_cast<uint16_t>(lenBytes[0]) |
            (static_cast<uint16_t>(lenBytes[1]) << 8);
        if (frameSize == 0 || frameSize > 6144) {
            return -1;
        }

        const size_t got = readBytes(buffer, frameSize);
        if (got != frameSize) {
            return -1;
        }

        return frameSize;
    }

    return -1;
}
