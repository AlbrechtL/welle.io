#
/*
 *    Copyright (C) 2013
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
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
 *	Experimental setup for using the SPIRAL generated viterbi
 *	decoder
 */
#include	<stdio.h>
#include	<stdlib.h>
#include	"mm_malloc.h"
#include	"viterbi.h"
#include	<cstring>

//
//	I really had problems with the _aligned_malloc function in MINGW,
//	so in the end I copied an implementation, found on the internet
//	https://sites.google.com/site/ruslancray/lab/bookshelf/interview/ci/low-level/write-an-aligned-malloc-free-function
//
#ifndef	__MINGW32__
//	in stdlib.h we should find
extern	int32_t	posix_memalign (void **memptr, size_t alignment, size_t size)throw ();
#else
void	*local_aligned_malloc (size_t required_bytes, size_t alignment) {
void	*p1;		// original block
void	**p2;		// aligned block
int	offset		= alignment - 1 + sizeof (void *);
	if ((p1 = (void *)malloc (required_bytes + offset)) == NULL) 
	   return NULL;

	p2	= (void **)(((size_t)(p1) + offset) & ~(alignment - 1));
	p2 [-1] = p1;
	return p2;
}

void	local_aligned_free (void *p) {
	free (((void **)p) [-1]);
}

#endif
//
//	Oh, Oh, Oh
//	It took a while to discover that the polynomes we used
//	in our own "straightforward" implementation was bitreversed!!
//	The official one is on top.
#define K 7
#define POLYS { 0155, 0117, 0123, 0155}
//#define	POLYS	{109, 79, 83, 109}
// In the reversed form the polys look:
//#define POLYS { 0133, 0171, 0145, 0133 }
//#define POLYS { 91, 121, 101, 91 }

#define	METRICSHIFT	0
#define	PRECISIONSHIFT	0
#define	RENORMALIZE_THRESHOLD	137

//
/* ADDSHIFT and SUBSHIFT make sure that the thing returned is a byte. */
#if (K-1<8)
#define ADDSHIFT (8-(K-1))
#define SUBSHIFT 0
#elif (K-1>8)
#define ADDSHIFT 0
#define SUBSHIFT ((K-1)-8)
#else
#define ADDSHIFT 0
#define SUBSHIFT 0
#endif

/* Create 256-entry odd-parity lookup table
 * Needed only on non-ia32 machines
 */
void viterbi::partab_init(void){
int i,cnt,ti;

/* Initialize parity lookup table */
	for (i = 0; i < 256; i++){
	   cnt = 0;
	   ti = i;
	   while (ti) {
	      if (ti & 1) cnt++;
	      ti >>= 1;
	   }
	   Partab [i] = cnt & 1;
	}
}

//int	viterbi::parityb (uint8_t x){
//	return Partab[x];
//}

int 	viterbi::parity (int x){
	/* Fold down to one byte */
	x ^= (x >> 16);
	x ^= (x >> 8);
	return Partab [x];
//	return parityb(x);
}

static inline
void	renormalize (COMPUTETYPE* X, COMPUTETYPE threshold){
int32_t	i;

	if (X[0] > threshold){
	   COMPUTETYPE min = X [0];
	   for (i = 0; i < NUMSTATES; i++)
	      if (min > X[i])
	         min = X[i];
	   for (i = 0; i < NUMSTATES; i++)
	      X[i] -= min;
      }
}

/**
  *	\class viterbi
  *	Implementation is borrowed from spiral
  */
	viterbi::viterbi		(int16_t wordlength) {
int polys [RATE] = POLYS;

	frameBits	= wordlength;
int16_t	i, state;
#ifdef	__MINGW32__
uint32_t	size;
#endif
	partab_init	();

#ifdef __MINGW32__
	size = ((wordlength + (K - 1)) / 8 + 1 + 16) & ~0xF;
	data	= (uint8_t *)local_aligned_malloc (size, 16);
#else
	if (posix_memalign ((void**)&data, 16,
	                        (wordlength + (K - 1))/ 8 + 1)){
	   printf("Allocation of data array failed\n");
	}
#endif

#ifdef	__MINGW32__
	size = (RATE * (wordlength + (K - 1)) * sizeof(COMPUTETYPE) + 16) & ~0xF;
	symbols	= (COMPUTETYPE *)local_aligned_malloc (size, 16);
#else
	if (posix_memalign ((void**)&symbols, 16,
	                     RATE * (wordlength + (K - 1)) * sizeof(COMPUTETYPE))){
	   printf("Allocation of symbols array failed\n");
	}
#endif

	for (state = 0; state < NUMSTATES / 2; state++) {
	   for (i = 0; i < RATE; i++)
	      Branchtab [i * NUMSTATES / 2 + state] =
	                     (polys[i] < 0) ^
	                        parity((2 * state) & abs (polys[i])) ? 255 : 0;
	}
//
// B I G N O T E	The spiral code uses (wordLength + (K - 1) * sizeof ...
// However, the application then crashes, so something is not OK
// By doubling the size, the problem disappears. It is not solved though
// and not further investigation.
#ifdef	__MINGW32__
	size	= 2 * (wordlength + (K - 1)) * sizeof (decision_t);	
	size	= (size + 16) & ~0xF;
	vp. decisions = (decision_t  *)local_aligned_malloc (size, 16);
#else
	if (posix_memalign ((void**)&(vp. decisions),
	                    16,
	                    2 * (wordlength + (K - 1)) * sizeof (decision_t))){
	   printf ("Allocation of vp decisions failed\n");
	}
#endif
	init_viterbi (&vp, 0);
}


	viterbi::~viterbi	(void) {
#ifdef	__MINGW32__
	local_aligned_free (vp. decisions);
	local_aligned_free (data);
	local_aligned_free (symbols);
#else
	free (vp. decisions);
	free (data);
	free (symbols);
#endif
}

uint8_t getbit (uint8_t v, int32_t o) {
uint8_t	mask	= 1 << (7 - o);
	return  (v & mask) ? 1 : 0;
}
	
// depends: POLYS, RATE, COMPUTETYPE
// 	encode was only used for testing purposes
//void encode (/*const*/ unsigned char *bytes, COMPUTETYPE *symbols, int nbits) {
//int	i, k;
//int	polys [RATE] = POLYS;
//int	sr = 0;
//
//// FIXME: this is slowish
//// -- remember about the padding!
//	for (i = 0; i < nbits + (K - 1); i++) {
//	   int b = bytes[i/8];
//	   int j = i % 8;
//	   int bit = (b >> (7-j)) & 1;
//
//	   sr = (sr << 1) | bit;
//	   for (k = 0; k < RATE; k++)
//	      *(symbols++) = parity(sr & polys[k]);
//	}
//}

//	Note that our DAB environment maps the softbits to -127 .. 127
//	we have to map that onto 0 .. 255

void	viterbi::deconvolve	(int16_t *input, uint8_t *output) {
uint32_t	i;

	init_viterbi (&vp, 0);
	for (i = 0; i < (uint16_t)(frameBits + (K - 1)) * RATE; i ++) {
	   int16_t temp = input [i] + 127;
	   if (temp < 0) temp = 0;
	   if (temp > 255) temp = 255;
	   symbols [i] = temp;
	}
//	update_viterbi_blk_GENERIC (&vp, symbols, frameBits + (K - 1));
	update_viterbi_blk_SPIRAL (&vp, symbols, frameBits + (K - 1));
	chainback_viterbi (&vp, data, frameBits, 0);

	for (i = 0; i < (uint16_t)frameBits; i ++)
	   output [i] = getbit (data [i >> 3], i & 07);
}

/* C-language butterfly */
void	viterbi::BFLY (int i, int s, COMPUTETYPE * syms,
	               struct v * vp, decision_t * d) {
int32_t j, decision0, decision1;
COMPUTETYPE metric,m0,m1,m2,m3;

	metric = 0;
	for (j = 0; j < RATE;j++)
	   metric += (Branchtab [i + j * NUMSTATES/2] ^ syms[s*RATE+j]) >>
	                                                     METRICSHIFT ;
	metric = metric >> PRECISIONSHIFT;
	const COMPUTETYPE max =
	        ((RATE * ((256 - 1) >> METRICSHIFT)) >> PRECISIONSHIFT);
	  
	m0 = vp->old_metrics->t [i] + metric;
	m1 = vp->old_metrics->t [i + NUMSTATES / 2] + (max - metric);
	m2 = vp->old_metrics->t [i] + (max - metric);
	m3 = vp->old_metrics->t [i + NUMSTATES / 2] + metric;
	  
	decision0 = ((int32_t)(m0 - m1)) > 0;
	decision1 = ((int32_t)(m2 - m3)) > 0;
	  
	vp -> new_metrics-> t[2 * i] = decision0 ? m1 : m0;
	vp -> new_metrics-> t[2 * i + 1] =  decision1 ? m3 : m2;
	  
	d -> w[i/(sizeof(uint32_t)*8/2)+s*(sizeof(decision_t)/sizeof(uint32_t))] |= 
		    (decision0|decision1<<1) << ((2*i)&(sizeof(uint32_t)*8-1));
}

/* Update decoder with a block of demodulated symbols
 * Note that nbits is the number of decoded data bits, not the number
 * of symbols!
 */
void	viterbi::update_viterbi_blk_GENERIC (struct v *vp,
					    COMPUTETYPE *syms, int16_t nbits){
decision_t *d = (decision_t *)vp -> decisions;
int32_t  s, i;

	for (s = 0; s < nbits; s++)
	   memset (&d [s], 0, sizeof (decision_t));

	for (s = 0; s < nbits; s++){
	   void *tmp;
	   for (i = 0; i < NUMSTATES / 2; i++)
	      BFLY (i, s, syms, vp, vp -> decisions);

	   renormalize (vp -> new_metrics -> t, RENORMALIZE_THRESHOLD);
//     Swap pointers to old and new metrics
	   tmp = vp -> old_metrics;
	   vp -> old_metrics = vp -> new_metrics;
	   vp -> new_metrics = (metric_t *)tmp;
	}
}
/**
  *	The "fast" implementation uses SSE instructions
  *	The configuration file lets you define (or undefine)
  *	that
  */
extern "C" {
#ifndef	SSE_AVAILABLE
void FULL_SPIRAL_no_sse (int, 
#else
void	FULL_SPIRAL_sse (int,
#endif
	                 COMPUTETYPE *Y,
	                 COMPUTETYPE *X,
	                 COMPUTETYPE *syms,
	                 DECISIONTYPE *dec,
	                 COMPUTETYPE *Branchtab);
}

void	viterbi::update_viterbi_blk_SPIRAL (struct v *vp,
					    COMPUTETYPE *syms,
					    int16_t nbits){
decision_t *d = (decision_t *)vp -> decisions;
int32_t s;

	for (s = 0; s < nbits; s++)
	   memset (d + s, 0, sizeof(decision_t));

#ifndef	SSE_AVAILABLE
	FULL_SPIRAL_no_sse (nbits,
#else
	FULL_SPIRAL_sse (nbits,
#endif
	                 vp -> new_metrics -> t,
	                 vp -> old_metrics -> t,
	                 syms,
	                 d -> t, Branchtab);
}

//
/* Viterbi chainback */
void	viterbi::chainback_viterbi (struct v *vp,
	                            uint8_t *data, /* Decoded output data */
	                            int16_t nbits, /* Number of data bits */
	                            uint16_t endstate){ /*Terminal encoder state */
decision_t *d = vp -> decisions;

/*	Make room beyond the end of the encoder register so we can
 *	accumulate a full byte of decoded data
 */
	endstate = (endstate % NUMSTATES) << ADDSHIFT;
/*	The store into data[] only needs to be done every 8 bits.
 *	But this avoids a conditional branch, and the writes will
 *	combine in the cache anyway
 */
	d += (K - 1); /* Look past tail */
	while (nbits-- != 0){
	   int k;
//	   int l	= (endstate >> ADDSHIFT) / 32;
//	   int m	= (endstate >> ADDSHIFT) % 32;
	   k = (d [nbits].w [(endstate >> ADDSHIFT) / 32] >>
	                       ((endstate>>ADDSHIFT) % 32)) & 1;
	   endstate = (endstate >> 1) | (k << (K - 2 + ADDSHIFT));
	   data [nbits >> 3] = endstate >> SUBSHIFT;
	}
}

/* Initialize Viterbi decoder for start of new frame */
void 	viterbi::init_viterbi (struct v *p, int16_t starting_state){
struct v *vp = p;
int32_t i;

	for (i = 0; i < NUMSTATES; i++)
	   vp -> metrics1.t[i] = 63;

	vp -> old_metrics = &vp -> metrics1;
	vp -> new_metrics = &vp -> metrics2;
/* Bias known start state */
	vp -> old_metrics-> t[starting_state & (NUMSTATES-1)] = 0;
}

