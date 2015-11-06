#
//
//	For the different formats for input, we have
//	different readers, with one "mother" reader.
//	Note that the cardreader is quite different here
//
//	This is the - almost empty - default implementation
#include	"virtual-reader.h"

	virtualReader::virtualReader	(RingBuffer<DSPCOMPLEX> *p, int32_t rate) {
	theBuffer	= p;
	blockSize	= -1;
	setMapper (rate, 2048000);
}

	virtualReader::~virtualReader		(void) {
}

void	virtualReader::restartReader	(int32_t s) {
	fprintf (stderr, "Restart met block %d\n", s);
	blockSize	= s;
}

void	virtualReader::stopReader	(void) {
}

void	virtualReader::processData	(float IQoffs, void *data, int cnt) {
	(void)IQoffs;
	(void)data;
	(void)cnt;
}

int16_t	virtualReader::bitDepth	(void) {
	return 12;
}

void	virtualReader::setMapper	(int32_t inRate, int32_t outRate) {
int32_t	i;

	this	-> inSize	= inRate / 1000;
	this	-> outSize	= outRate / 1000;
	inTable		= new DSPCOMPLEX [inSize];
	outTable	= new DSPCOMPLEX [outSize];
	mapTable	= new float [outSize];
	for (i = 0; i < outSize; i ++)
	   mapTable [i] = (float) i * inRate / outRate;
	conv	= 0;
}

void	virtualReader::convertandStore (DSPCOMPLEX *s, int32_t amount) {
int32_t	i, j;

	for (i = 0; i < amount; i ++) {
	   inTable [conv++]	= s [i];
	   if (conv >= inSize) {	// full buffer, map
	      for (j = 0; j < outSize - 1; j ++) {
	         int16_t base	= (int)(floor (mapTable [j]));
	         float  frac	= mapTable [j] - base;
	         outTable [j]	= cmul (inTable [base], 1 - frac) +
	                          cmul (inTable [base + 1], frac);
	      }
	      
//
//	let op, het laatste element was nog niet gebruikta
	      conv	= 1;
	      inTable [0] = inTable [inSize - 1];
	      theBuffer -> putDataIntoBuffer (outTable, outSize - 1);
	   }
	}
}

