#
//
//	For the different formats for input, we have
//	different readers, with one "mother" reader.
//	Note that the cardreader is quite different here
#ifndef	__VIRTUAL_READER
#define	__VIRTUAL_READER

#include	<stdint.h>
#include	<stdio.h>
#include	"ringbuffer.h"
#include	"dab-constants.h"
//
//	The virtualReader is the mother of the readers.
//	The cardReader is slighty different, however
//	made fitting the framework
class	virtualReader {
protected:
RingBuffer<DSPCOMPLEX>	*theBuffer;
int32_t	blockSize;
public:
		virtualReader	(RingBuffer<DSPCOMPLEX> *p, int32_t rate);
virtual		~virtualReader	(void);
virtual void	restartReader	(int32_t s);
virtual void	stopReader	(void);
virtual void	processData	(float IQoffs, void *data, int cnt);
virtual	int16_t	bitDepth	(void);
protected:
	int32_t	base;
	void	convertandStore		(DSPCOMPLEX *, int32_t);
private:
	void	setMapper	(int32_t, int32_t);
	float	*mapTable;
	int16_t	conv;
	int16_t	inSize;
	int16_t	outSize;
	DSPCOMPLEX	*inTable;
	DSPCOMPLEX	*outTable;
	
};

#endif

