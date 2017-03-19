/*
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

#include <QDebug>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "CRAWFile.h"

static inline int64_t getMyTime(void)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return ((int64_t)tv.tv_sec * 1000000 + (int64_t)tv.tv_usec);
}

#define INPUT_FRAMEBUFFERSIZE 8 * 32768

CRAWFile::CRAWFile()
{
    FileName = "";
    FileFormat = CRAWFileFormat::Unknown;
    IQByteSize = 1;

    readerOK = false;

    SampleBuffer = new RingBuffer<uint8_t>(INPUT_FRAMEBUFFERSIZE);
    SpectrumSampleBuffer = new RingBuffer<uint8_t>(8192);
}

CRAWFile::~CRAWFile(void)
{
    ExitCondition = true;
    if (readerOK) {
        while (isRunning())
            usleep(100);
        fclose(filePointer);
        delete SampleBuffer;
        delete SpectrumSampleBuffer;
    }
}

void CRAWFile::setFrequency(int32_t Frequency)
{
    (void)Frequency;
}

bool CRAWFile::restart(void)
{
    if (readerOK)
        readerPausing = false;
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

float CRAWFile::setGain(int32_t Gain)
{
    (void)Gain;

    return 0;
}

int32_t CRAWFile::getGainCount()
{
    return 0;
}

void CRAWFile::setAgc(bool AGC)
{
    (void)AGC;
}

QString CRAWFile::getName()
{
    return "rawfile (" + FileName + ")";
}

CDeviceID CRAWFile::getID()
{
    return CDeviceID::RAWFILE;
}

void CRAWFile::setFileName(QString FileName, QString FileFormat)
{
    this->FileName = FileName;

    if(FileFormat == "u8")
    {
        this->FileFormat = CRAWFileFormat::U8;
        IQByteSize = 2;
    }
    else if(FileFormat == "s16le")
    {
        this->FileFormat = CRAWFileFormat::S16LE;
        IQByteSize = 4;
    }
    else
    {
        this->FileFormat = CRAWFileFormat::Unknown;
        qDebug() << "RAWFile:"
                 << "Unknown RAW file format:" << FileFormat;
    }

    filePointer = fopen(FileName.toLatin1().data(), "rb");
    if (filePointer == NULL) {
        qDebug() << "RAWFile:"
                 << "file" << FileName << "cannot open";
        return;
    }

    readerOK = true;
    readerPausing = true;
    currPos = 0;
    start();
}

//	size is in I/Q pairs, file contains 8 bits values
int32_t CRAWFile::getSamples(DSPCOMPLEX* V, int32_t size)
{
    int32_t amount, i;
    uint8_t* temp = (uint8_t*)alloca(IQByteSize * size * sizeof(uint8_t));

    if (filePointer == NULL)
        return 0;

    while ((int32_t)(SampleBuffer->GetRingBufferReadAvailable()) < IQByteSize * size)
        if (readerPausing)
            usleep(100000);
        else
            msleep(100);

    amount = SampleBuffer->getDataFromBuffer(temp, IQByteSize * size);

    if(FileFormat == CRAWFileFormat::U8)
    {
        for (i = 0; i < amount / 2; i++)
        V[i] = DSPCOMPLEX(float(temp[2 * i] - 128) / 128.0,
        float(temp[2 * i + 1] - 128) / 128.0);
    }
    else if(FileFormat == CRAWFileFormat::S16LE)
    {
        int j=0;
        for (i = 0; i < amount / 4; i++)
        {
            int16_t IQ_I = (int16_t) (temp[j + 0] << 8) | temp[j + 1];
            int16_t IQ_Q = (int16_t) (temp[j + 2] << 8) | temp[j + 3];
            V[i] = DSPCOMPLEX((float) (IQ_I ), (float) (IQ_Q ));

            j +=IQByteSize;
        }
    }

    return amount / IQByteSize;
}

int32_t CRAWFile::getSpectrumSamples(DSPCOMPLEX* V, int32_t size)
{
    int32_t amount, i;
    uint8_t* temp = (uint8_t*)alloca(IQByteSize * size * sizeof(uint8_t));

    amount = SpectrumSampleBuffer->getDataFromBuffer(temp, IQByteSize * size);

    if(FileFormat == CRAWFileFormat::U8)
    {
        for (i = 0; i < amount / 2; i++)
        V[i] = DSPCOMPLEX(float(temp[2 * i] - 128) / 128.0,
        float(temp[2 * i + 1] - 128) / 128.0);
    }
    else if(FileFormat == CRAWFileFormat::S16LE)
    {
        int j=0;
        for (i = 0; i < amount / 4; i++)
        {
            int16_t IQ_I = (int16_t) (temp[j + 0] << 8) | temp[j + 1];
            int16_t IQ_Q = (int16_t) (temp[j + 2] << 8) | temp[j + 3];
            V[i] = DSPCOMPLEX((float) (IQ_I ), (float) (IQ_Q ));

            j +=IQByteSize;
        }
    }

    return amount / IQByteSize;
}

int32_t CRAWFile::getSamplesToRead(void)
{
    return SampleBuffer->GetRingBufferReadAvailable() / 2;
}

void CRAWFile::run(void)
{
    int32_t t, i;
    uint8_t* bi;
    int32_t bufferSize = 32768;
    int32_t period;
    int64_t nextStop;

    if (!readerOK)
        return;

    ExitCondition = false;

    period = (32768 * 1000) / (IQByteSize * 2048); // full IQś read

    qDebug() << "RAWFile"
             << "Period =" << period;
    bi = new uint8_t[bufferSize];
    nextStop = getMyTime();
    while (!ExitCondition) {
        if (readerPausing) {
            usleep(1000);
            nextStop = getMyTime();
            continue;
        }

        while (SampleBuffer->WriteSpace() < bufferSize + 10) {
            if (ExitCondition)
                break;
            usleep(100);
        }

        nextStop += period;
        t = readBuffer(bi, bufferSize);
        if (t <= 0) {
            for (i = 0; i < bufferSize; i++)
                bi[i] = 0;
            t = bufferSize;
        }
        SampleBuffer->putDataIntoBuffer(bi, t);
        SpectrumSampleBuffer->putDataIntoBuffer(bi, t);
        if (nextStop - getMyTime() > 0)
            usleep(nextStop - getMyTime());
    }

    qDebug() << "RAWFile:" <<  "Read threads ends";
}

/*
 *	length is number of uints that we read.
 */
int32_t CRAWFile::readBuffer(uint8_t* data, int32_t length)
{
    int32_t n;

    n = fread(data, sizeof(uint8_t), length, filePointer);
    currPos += n;
    if (n < length) {
        fseek(filePointer, 0, SEEK_SET);
       qDebug() << "RAWFile:" <<  "End of file, restarting";
    }
    return n & ~01;
}
