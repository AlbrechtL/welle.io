/******************************************************************************
** kjmp2 -- a minimal MPEG-1 Audio Layer II decoder library                  **
*******************************************************************************
** Copyright (C) 2006 Martin J. Fiedler <martin.fiedler@gmx.net>             **
**                                                                           **
** This software is provided 'as-is', without any express or implied         **
** warranty. In no event will the authors be held liable for any damages     **
** arising from the use of this software.                                    **
**                                                                           **
** Permission is granted to anyone to use this software for any purpose,     **
** including commercial applications, and to alter it and redistribute it    **
** freely, subject to the following restrictions:                            **
**   1. The origin of this software must not be misrepresented; you must not **
**      claim that you wrote the original software. If you use this software **
**      in a product, an acknowledgment in the product documentation would   **
**      be appreciated but is not required.                                  **
**   2. Altered source versions must be plainly marked as such, and must not **
**      be misrepresented as being the original software.                    **
**   3. This notice may not be removed or altered from any source            **
**      distribution.                                                        **
******************************************************************************/
//
//  This software is a rewrite of the original kjmp2 software,
//  Rewriting in the form of a class
//  for use in the sdr-j DAB/DAB+ receiver
//  all rights remain where they belong
#ifndef MP2PROCESSOR
#define MP2PROCESSOR

#include    <stdio.h>
#include    <stdint.h>
#include    <memory>
#include    <math.h>
#include    "dab-processor.h"
#include    <QObject>
#include    <vector>
#include    <stdio.h>
#include    "ringbuffer.h"
#include    "pad_decoder_adapter.h"

#define KJMP2_MAX_FRAME_SIZE    1440  // the maximum size of a frame
#define KJMP2_SAMPLES_PER_FRAME 1152  // the number of samples per frame

// quantizer specification structure
struct quantizer_spec
{
    int32_t nlevels;
    uint8_t grouping;
    uint8_t cw_bits;
};

class CRadioController;

class mp2Processor: public QObject, public dabProcessor
{
    Q_OBJECT
    public:
        mp2Processor( CRadioController *mr,
                      int16_t bitRate,
                      RingBuffer<int16_t> *buffer);
        ~mp2Processor(void);
        virtual void addtoFrame(uint8_t *v);
        void setFile(FILE *);

    private:
        int32_t     mp2sampleRate(uint8_t *frame);
        int32_t     mp2decodeFrame(uint8_t *frame, int16_t *pcm);

        CRadioController    *myRadioInterface;
        RingBuffer<int16_t> *buffer;
        int16_t     bitRate;
        int32_t     baudRate;
        void        setSamplerate(int32_t rate);
        struct quantizer_spec *read_allocation(int sb, int b2_table);
        void        read_samples(struct quantizer_spec *q,
                                int scalefactor,
                                int *sample);
        int32_t     get_bits(int32_t bit_count);
        int16_t     V[2][1024];
        int16_t     Voffs;
        int16_t     N[64][32];
        struct quantizer_spec *allocation[2][32];
        int32_t     scfsi[2][32];
        int32_t     scalefactor[2][32][3];
        int32_t     sample[2][32][3];
        int32_t     U[512];

        int32_t     bit_window;
        int32_t     bits_in_window;
        uint8_t     *frame_pos;
        std::vector<uint8_t> MP2frame;
        int16_t     MP2framesize;
        int16_t     MP2Header_OK;
        int16_t     MP2headerCount;
        int16_t     MP2bitCount;
        void        addbittoMP2 (uint8_t *v, uint8_t b, int16_t nm);
        int16_t     numberofFrames;
        int16_t     errorFrames;
        std::unique_ptr<PADDecoderAdapter> padDecoderAdapter;

        QByteArray  *MP2FileName;
        FILE *MP2File;

    signals:
        void        show_frameErrors(int errorFrames);
        void        newAudio(int baudRate);
        void        isStereo(bool isJointStereo);
};
#endif

