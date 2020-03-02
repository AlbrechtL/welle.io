/*
 *    Copyright (C) 2018
 *    Matthias P. Braendli (matthias.braendli@mpb.li)
 *
 *    Copyright (C) 2017
 *    Albrecht Lohofener (albrechtloh@gmx.de)
 *
 *    This file is based on SDR-J
 *    Copyright (C) 2010, 2011, 2012
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
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

#include <string>
#include <iostream>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "raw_file.h"

// For Qt translation if Qt is exisiting
#ifdef QT_CORE_LIB
    #include <QtGlobal>
#else
    #define QT_TRANSLATE_NOOP(x,y) (y)
#endif

static inline int64_t getMyTime(void)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return ((int64_t)tv.tv_sec * 1000000 + (int64_t)tv.tv_usec);
}

#define INPUT_FRAMEBUFFERSIZE 8 * 32768

CRAWFile::CRAWFile(RadioControllerInterface& radioController,
        bool throttle, bool rewind) :
    radioController(radioController),
    throttle(throttle),
    autoRewind(rewind),
    fileName(""),
    fileFormat(CRAWFileFormat::Unknown),
    IQByteSize(1),
    SampleBuffer(INPUT_FRAMEBUFFERSIZE),
    SpectrumSampleBuffer(8*2048)
{
}

CRAWFile::~CRAWFile(void)
{
    ExitCondition = true;
    if (readerOK) {
        if (thread.joinable()) {
            thread.join();
        }

        if (filePointer) {
            fclose(filePointer);
        }
    }
}

void CRAWFile::setFrequency(int Frequency)
{
    (void)Frequency;
}

int CRAWFile::getFrequency() const
{
    return 0;
}

bool CRAWFile::restart(void)
{
    if (readerOK)
        readerPausing = false;
    return readerOK;
}

bool CRAWFile::is_ok()
{
    return readerOK;
}

void CRAWFile::stop(void)
{
    if (readerOK)
        readerPausing = true;
}

void CRAWFile::reset()
{
}

void CRAWFile::rewind()
{
    if (filePointer) {
        fseek(filePointer, 0, SEEK_SET);
        endReached = false;
    }
}

float CRAWFile::getGain() const
{
    return 0;
}

float CRAWFile::setGain(int Gain)
{
    (void)Gain;

    return 0;
}

int CRAWFile::getGainCount()
{
    return 0;
}

void CRAWFile::setAgc(bool AGC)
{
    (void)AGC;
}

std::string CRAWFile::getDescription()
{
    return "rawfile (" + fileName + ")";
}

CDeviceID CRAWFile::getID()
{
    return CDeviceID::RAWFILE;
}

bool ends_with(const std::string& value, const std::string& ending)
{
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

void CRAWFile::setFileName(const std::string& fileName,
        const std::string& fileFormat)
{
    this->fileName = fileName;

    setFileFormat(fileFormat);

    filePointer = fopen(fileName.c_str(), "rb");
    if (filePointer == nullptr) {
        std::clog << "RAWFile: Cannot open file: " << fileName << std::endl;
        radioController.onMessage(message_level_t::Error,
                QT_TRANSLATE_NOOP("CRadioController", "Cannot open file "), fileName);
        return;
    }

    readerOK = true;
    readerPausing = true;
    currPos = 0;
    thread = std::thread(&CRAWFile::run, this);
}

void CRAWFile::setFileHandle(int handle, const std::string& fileFormat)
{
    this->fileName = "unknown";

    setFileFormat(fileFormat);

    filePointer = fdopen(handle, "rb");
    if (filePointer == nullptr) {
        std::clog << "RAWFile: Cannot open file: " << fileName << std::endl;
        radioController.onMessage(message_level_t::Error,
                QT_TRANSLATE_NOOP("CRadioController", "Cannot open file "), fileName);
        return;
    }

    readerOK = true;
    readerPausing = true;
    currPos = 0;
    thread = std::thread(&CRAWFile::run, this);
}

std::string CRAWFile::getFileName() const
{
    return fileName;
}

//	size is in I/Q pairs, file contains 8 bits values
int32_t CRAWFile::getSamples(DSPCOMPLEX* V, int32_t size)
{
    if (filePointer == nullptr)
        return 0;

    while ((int32_t)(SampleBuffer.GetRingBufferReadAvailable()) < IQByteSize * size)
        if (readerPausing)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

    return convertSamples(SampleBuffer, V, size);
}

std::vector<DSPCOMPLEX> CRAWFile::getSpectrumSamples(int size)
{
    std::vector<DSPCOMPLEX> buffer(size);

    int sizeRead = convertSamples(SpectrumSampleBuffer, buffer.data(), size);
    if (sizeRead < size) {
        buffer.resize(sizeRead);
    }

    return buffer;
}

int32_t CRAWFile::getSamplesToRead(void)
{
    return SampleBuffer.GetRingBufferReadAvailable() / 2;
}

void CRAWFile::run(void)
{
    int32_t t;
    int32_t bufferSize = 32768;
    int32_t period;
    int64_t nextStop;

    if (!readerOK)
        return;

    ExitCondition = false;

    period = (32768 * 1000) / (IQByteSize * 2048); // full IQs read

    std::clog << "RAWFile" << "Period =" << period << std::endl;
    std::vector<uint8_t> bi(bufferSize);
    nextStop = getMyTime();
    while (!ExitCondition) {
        if (readerPausing) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            nextStop = getMyTime();
            continue;
        }

        while (SampleBuffer.WriteSpace() < bufferSize + 10) {
            if (ExitCondition)
                break;
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }

        nextStop += period;
        t = readBuffer(bi.data(), bufferSize);
        if (t <= 0) {
            for (int i = 0; i < bufferSize; i++)
                bi[i] = 0;
            t = bufferSize;
        }
        SampleBuffer.putDataIntoBuffer(bi.data(), t);
        SpectrumSampleBuffer.putDataIntoBuffer(bi.data(), t);
        putIntoRecordBuffer(*bi.data(), t);
        int64_t t_to_wait = nextStop - getMyTime();
        if (throttle and t_to_wait > 0)
            std::this_thread::sleep_for(std::chrono::microseconds(t_to_wait));
    }

    std::clog << "RAWFile:" <<  "Read threads ends" << std::endl;
}

/*
 *	length is number of uints that we read.
 */
int32_t CRAWFile::readBuffer(uint8_t* data, int32_t length)
{
    int32_t n;

    if (!filePointer) {
        return 0;
    }

    n = fread(data, sizeof(uint8_t), length, filePointer);
    currPos += n;
    if (n < length) {
        if (autoRewind) {
            fseek(filePointer, 0, SEEK_SET);
            std::clog << "RAWFile:"  << "End of file, restarting" << std::endl;
            radioController.onMessage(message_level_t::Information,
                    QT_TRANSLATE_NOOP("CRadioController", "End of file, restarting"));
        }
        else {
            radioController.onMessage(message_level_t::Information, QT_TRANSLATE_NOOP("CRadioController", "End of file"));
            endReached = true;
            return 0;
        }
    }
    return n & ~01;
}

int32_t CRAWFile::convertSamples(RingBuffer<uint8_t>& Buffer, DSPCOMPLEX *V, int32_t size)
{
    // Native endianness complex<float> requires no conversion
    if (fileFormat == CRAWFileFormat::COMPLEXF) {
        int32_t amount = Buffer.getDataFromBuffer(V, IQByteSize * size);
        return amount / IQByteSize;
    }

    std::vector<uint8_t> temp((size_t)IQByteSize * (size_t)size);

    int32_t amount = Buffer.getDataFromBuffer(temp.data(), IQByteSize * size);

    // Unsigned 8-bit
    if (fileFormat == CRAWFileFormat::U8) {
        for (int i = 0; i < amount / 2; i++)
            V[i] = DSPCOMPLEX(float(temp[2 * i] - 128) / 128.0,
                              float(temp[2 * i + 1] - 128) / 128.0);
    }
    // Signed 8-bit
    else if (fileFormat == CRAWFileFormat::S8) {
        for (int i = 0; i < amount / 2; i++)
            V[i] = DSPCOMPLEX(float((int8_t)temp[2 * i]) / 128.0,
                              float((int8_t)temp[2 * i + 1]) / 128.0);
    }
    // Signed 16-bit little endian
    else if (fileFormat == CRAWFileFormat::S16LE) {
        for (int i = 0, j = 0; i < amount / 4; i++, j+= IQByteSize) {
            int16_t IQ_I = (int16_t)(temp[j + 0] << 8) | temp[j + 1];
            int16_t IQ_Q = (int16_t)(temp[j + 2] << 8) | temp[j + 3];
            V[i] = DSPCOMPLEX((float)(IQ_I), (float)(IQ_Q));
        }
    }
    // Signed 16-bit big endian
    else if (fileFormat == CRAWFileFormat::S16BE) {
        for (int i = 0, j = 0; i < amount / 4; i++, j += IQByteSize) {
            int16_t IQ_I = (int16_t)(temp[j + 1] << 8) | temp[j + 0];
            int16_t IQ_Q = (int16_t)(temp[j + 3] << 8) | temp[j + 2];
            V[i] = DSPCOMPLEX((float)(IQ_I), (float)(IQ_Q));
        }
    }

    return amount / IQByteSize;
}

void CRAWFile::setFileFormat(const std::string &fileFormat)
{
    if (fileFormat == "u8" or
            (fileFormat == "auto" and ends_with(fileName, ".u8.iq"))) {
        this->fileFormat = CRAWFileFormat::U8;
        IQByteSize = 2;
    }
    else if (fileFormat == "s8" or
            (fileFormat == "auto" and ends_with(fileName, ".s8.iq"))) {
        this->fileFormat = CRAWFileFormat::S8;
        IQByteSize = 2;
    }
    else if(fileFormat == "s16le" or
            (fileFormat == "auto" and ends_with(fileName, ".s16le.iq"))) {
        this->fileFormat = CRAWFileFormat::S16LE;
        IQByteSize = 4;
    }
    else if(fileFormat == "s16be" or
            (fileFormat == "auto" and ends_with(fileName, ".s16be.iq"))) {
        this->fileFormat = CRAWFileFormat::S16BE;
        IQByteSize = 4;
    }
    else if(fileFormat == "cf32" or
            (fileFormat == "auto" and ends_with(fileName, ".cf32.iq"))) {
        this->fileFormat = CRAWFileFormat::COMPLEXF;
        IQByteSize = 8;
    }
    else if (fileFormat == "auto") {
        // Default to u8 for backward compatibility
        this->fileFormat = CRAWFileFormat::U8;
        IQByteSize = 2;
    }
    else {
        this->fileFormat = CRAWFileFormat::Unknown;
        std::clog << "RAWFile: unknown file format" << std::endl;
        radioController.onMessage(message_level_t::Error,
                QT_TRANSLATE_NOOP("CRadioController", "Unknown RAW file format"));
    }
}
