#
//
//	For the different formats for input, we have
//	different readers, with one "mother" reader.
//	Note that the cardreader is quite different here
//	and its code is elsewhere
#ifndef	__COMMON_READERS
#define	__COMMON_READERS

#include	"virtual-reader.h"

class	reader_16: public virtualReader {
public:
	reader_16	(RingBuffer<DSPCOMPLEX> *p, int32_t, int32_t);
	~reader_16	(void);
void	processData	(float IQoffs, void *data, int cnt);
int16_t bitDepth	(void);
};

class	reader_24: public virtualReader {
public:
	reader_24	(RingBuffer<DSPCOMPLEX> *p, int32_t, int32_t);
	~reader_24	(void);
void	processData	(float IQoffs, void *data, int cnt);
int16_t bitDepth	(void);
};

class	reader_32: public virtualReader {
public:
	reader_32	(RingBuffer<DSPCOMPLEX> *p, int32_t, int32_t);
	~reader_32	(void);
void	processData	(float IQoffs, void *data, int cnt);
int16_t	bitDepth	(void);
};

//
//	This is the only one we actually need for
//	elad s2 as input device for DAB
class	reader_float: public virtualReader {
public:
	reader_float	(RingBuffer<DSPCOMPLEX> *p, int32_t);
	~reader_float	(void);
void	processData	(float IQoffs, void *data, int cnt);
int16_t	bitDepth	(void);
private:
	int16_t		mapTable_int	[2048];
	float		mapTable_float	[2048];
	DSPCOMPLEX	convBuffer	[3072 + 1];
	int16_t		convIndex;
};

#endif

