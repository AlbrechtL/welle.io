/*
 *    Copyright (C) 2013
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the SDR-J (JSDR).
 *    SDR-J is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    SDR-J is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with SDR-J; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#ifndef __DAB_AUDIO
#define __DAB_AUDIO

#include    "dab-virtual.h"
#include    <memory>
#include    <atomic>
#include    <vector>
#include    <thread>
#include    <mutex>
#include    <condition_variable>
#include    "ringbuffer.h"
#include    <stdio.h>

class dabProcessor;
class protection;
class CRadioController;

class dabAudio : public dabVirtual
{
    public:
        dabAudio(uint8_t dabModus,
                  int16_t fragmentSize,
                  int16_t bitRate,
                  bool   shortForm,
                  int16_t protLevel,
                  CRadioController *mr,
                  std::shared_ptr<RingBuffer<int16_t> >);
        ~dabAudio(void);
        dabAudio(const dabAudio&) = delete;
        dabAudio& operator=(const dabAudio&) = delete;

        int32_t process(int16_t *v, int16_t cnt);

    protected:
        CRadioController    *myRadioInterface;
        std::shared_ptr<RingBuffer<int16_t>> audioBuffer;

    private:
        void    run(void);
        std::atomic<bool> running;
        uint8_t     dabModus;
        int16_t     fragmentSize;
        int16_t     bitRate;
        bool        shortForm;
        int16_t     protLevel;
        std::vector<uint8_t> outV;
        int16_t     **interleaveData;

        std::condition_variable  Locker;
        std::mutex               ourMutex;
        std::thread              ourThread;

        std::unique_ptr<protection> protectionHandler;
        std::unique_ptr<dabProcessor> our_dabProcessor;
        RingBuffer<int16_t> Buffer;
};

#endif

