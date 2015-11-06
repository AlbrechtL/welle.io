#
//
//	For the different formats for input, we have
//	different readers, with one "mother" reader.
//	Note that the cardreader is quite different here
#ifndef	__READER
#define	__READER

#include	<stdint.h>
#include	<stdio.h>
#include	"ringbuffer.h"
#include	"dab-constants.h"

class	reader {
protected:
RingBuffer<DSPCOMPLEX>	*theBuffer;
int32_t	blockSize;
public:
	reader		(RingBuffer<DSPCOMPLEX> *p) {
	theBuffer	= p;
	blockSize	= -1;
}

virtual	~reader		(void) {
}

virtual void	restartReader	(int32_t s) {
	blockSize	= s;
}

virtual void	stopReader	(void) {
}

virtual void	processData	(float IQoffs, void *data, int cnt) {
	(void)IQoffs;
	(void)data;
	(void)cnt;
}

virtual	int16_t	bitDepth	(void) {
	return 12;
}
};

class	reader_16: public reader {
public:
	reader_16 (RingBuffer<DSPCOMPLEX> *p):reader (p) {
}

	~reader_16 (void) {
}

void	processData	(float IQoffs, void *data, int cnt) {
int32_t	i;
DSPCOMPLEX temp [blockSize];
uint8_t	*p	= (uint8_t *)data;
	(void)IQoffs;
	for (i = 0; i < blockSize; i ++) {
	   uint8_t r0	= p [4 * i];
	   uint8_t r1	= p [4 * i + 1];
	   uint8_t i0	= p [4 * i + 2];
	   uint8_t i1	= p [4 * i + 3];
	   float re	= (r0 << 8 | r1) / 32767.0;
	   float im	= (i0 << 8 | i1) / 32767.0;
	   temp [i] = DSPCOMPLEX (re, im);
	}
	theBuffer	-> putDataIntoBuffer (temp, blockSize);
}

int16_t bitDepth	(void) {
	return 16;
}
};

class	reader_24: public reader {
public:
	reader_24 (RingBuffer<DSPCOMPLEX> *p):reader (p) {
}

	~reader_24 (void) {
}

void	processData	(float IQoffs, void *data, int cnt) {
int32_t	i;
DSPCOMPLEX temp [blockSize];
uint8_t	*p	= (uint8_t *)data;
	(void)IQoffs;
	for (i = 0; i < 6 * blockSize; i ++) {
	   uint8_t r0	= p [6 * i];
	   uint8_t r1	= p [6 * i + 1];
	   uint8_t r2	= p [6 * i + 2];
	   uint8_t i0	= p [6 * i + 3];
	   uint8_t i1	= p [6 * i + 4];
	   uint8_t i2	= p [6 * i + 5];
	   float re	= (r0 << 16 | r1 << 8 | r2) / (32768 * 256);
	   float im	= (i0 << 16 | i1 << 8 | i2) / (32768 * 256);
	   temp [i] = DSPCOMPLEX (re, im);
	}
	theBuffer	-> putDataIntoBuffer (temp, blockSize);
}

int16_t bitDepth	(void) {
	return 24;
}
};

class	reader_32: public reader {
public:
	reader_32 (RingBuffer<DSPCOMPLEX> *p):reader (p) {
}

	~reader_32 (void) {
}

void	processData	(float IQoffs, void *data, int cnt) {
int32_t	i;
DSPCOMPLEX temp [blockSize];
uint8_t	*p	= (uint8_t *)data;
	(void)IQoffs;
	for (i = 0; i < 8 * blockSize; i ++) {
	   uint8_t r0	= p [8 * i];
	   uint8_t r1	= p [8 * i + 1];
	   uint8_t r2	= p [8 * i + 2];
	   uint8_t r3	= p [8 * i + 3];
	   uint8_t i0	= p [8 * i + 4];
	   uint8_t i1	= p [8 * i + 5];
	   uint8_t i2	= p [8 * i + 6];
	   uint8_t i3	= p [8 * i + 7];
	   float re	= (r0 << 24 | r1 << 16 | r2 << 8 | r3) / (32767 * 65536);
	   float im	= (i0 << 24 | i1 << 16 | i2 << 8 | i3) / (32767 * 65336);
	   temp [i] = DSPCOMPLEX (re, im);
	}
	theBuffer	-> putDataIntoBuffer (temp, blockSize);
}

int16_t	bitDepth	(void) {
	return 32;
}
};


class	reader_float: public reader {
public:
	reader_float (RingBuffer<DSPCOMPLEX> *p):reader (p) {
}

	~reader_float (void) {
}

void	processData	(float IQoffs, void *data, int cnt) {
int32_t	i;
DSPCOMPLEX temp [blockSize];
float	*p	= (float *)data;
	(void)IQoffs;
	for (i = 0; i < 2 * blockSize; i ++) 
	   temp [i] = DSPCOMPLEX (p [2 * i], p [2 * i + 1]);
	theBuffer	-> putDataIntoBuffer (temp, blockSize);
}

int16_t bitDepth	(void) {
	return 32;
}
};

#endif

