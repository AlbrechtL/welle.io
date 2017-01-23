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
 *	We use the "Generic" implementation - i.e. the framework -
 *	as given by the Spiral Project. All rights gratefully acknowledged.
 *	decoder
 */
#include	<stdio.h>
#include	<stdlib.h>
#include	"mm_malloc.h"
#include	"viterbi.h"
#include	<cstring>
#ifdef	__MINGW32__
#include <stdlib.h>
#include <intrin.h>
#include <malloc.h>
#include <windows.h>
#endif

//
//	It took a while to discover that the polynomes I used
//	in a "home" made implementation was bitreversed!!
//	The official one is on top.
#define K 7
#define POLYS {0155, 0117, 0123, 0155}
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

static uint8_t Partab [] = 
{ 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0};

//
//	One could create the table above, i.e. a 256 entry
//	odd-parity lookup table by the following function
//	It is now precomputed
static
void	partab_init(void){
int16_t i,cnt,ti;

	for (i = 0; i < 256; i++){
	   cnt = 0;
	   ti = i;
	   while (ti != 0) {
	      if (ti & 1) cnt++;
	      ti >>= 1;
	   }
	   Partab [i] = cnt & 1;
	}
}

int16_t  viterbi::parity (int16_t x){
	/* Fold down to one byte */
	x ^= (x >> 8);
	return Partab [x];
}

static inline
void	renormalize (int16_t* X, int16_t threshold){
int32_t	i;

	if (X [0] > threshold){
	   int16_t min = X [0];
	   for (i = 0; i < NUMSTATES; i++)
	      if (min > X[i])
	         min = X[i];
	   for (i = 0; i < NUMSTATES; i++)
	      X[i] -= min;
      }
}

	viterbi::viterbi		(int16_t wordlength) {
int polys [RATE] = POLYS;
	frameBits	= wordlength;
int16_t	i, state;
#ifdef	__MINGW32__
uint32_t	size;
	size	= ((wordlength + (K - 1)) / 8 + 1 + 16) & ~0x0F;
	data	= (uint8_t *)_aligned_malloc (size, 16);
	size	= (RATE * (wordlength + (K - 1)) * sizeof (int16_t) + 1 + 16) & 0x0F;
	symbols	= (int16_t *)_aligned_malloc (size, 16);
	size	= ((wordlength + (K - 1)) * sizeof (decision_t) + 16) & ~0x0F;
	vp. decisions = (decision_t  *)_aligned_malloc (size, 16);
#else
	if (posix_memalign ((void**)&data, 16,
	                        (wordlength + (K - 1))/ 8 + 1)){
	   printf("Allocation of data array failed\n");
	}
	if (posix_memalign ((void**)&symbols, 16,
	                     RATE * (wordlength + (K - 1)) * sizeof(int16_t))){
	   printf("Allocation of symbols array failed\n");
	}
	if (posix_memalign ((void**)&(vp. decisions),
	                    16,
	                    2 * (wordlength + (K - 1)) * sizeof (decision_t))){
	   printf ("Allocation of vp decisions failed\n");
	}
#endif

	for (state = 0; state < NUMSTATES / 2; state++) {
	   for (i = 0; i < RATE; i++)
	      Branchtab [i * NUMSTATES / 2 + state] =
	                     (polys[i] < 0) ^
	                        parity((2 * state) & abs (polys[i])) ? 255 : 0;
	}
	init_viterbi (&vp, 0);
}


	viterbi::~viterbi	(void) {
#ifdef	__MINGW32__
	_aligned_free (vp. decisions);
	_aligned_free (data);
	_aligned_free (symbols);
#else
	free (vp. decisions);
	free (data);
	free (symbols);
#endif
}

static int maskTable [] = {128, 64, 32, 16, 8, 4, 2, 1};
static	inline
uint8_t getbit (uint8_t v, int32_t o) {
	return  (v & maskTable [o]) ? 1 : 0;
}
	
void	viterbi::deconvolve	(int16_t *input, uint8_t *output) {
int16_t	i;

	init_viterbi (&vp, 0);
	update_viterbi_blk_GENERIC (&vp, input, frameBits + (K - 1));
	chainback_viterbi (&vp, data, frameBits, 0);

	for (i = 0; i < (int16_t)frameBits; i ++)
	   output [i] = getbit (data [i >> 3], i & 07);
}

/* C-language butterfly */
void	viterbi::BFLY (int i, int s, int16_t * syms,
	               struct v * vp, decision_t * d) {
int32_t j, decision0, decision1;
int16_t metric, m0, m1, m2, m3;

	metric = 0;
	for (j = 0; j < RATE;j++)
	   metric += (Branchtab [i + j * NUMSTATES/2] ^ syms[s * RATE + j]) >>
	                                                     METRICSHIFT ;
	metric = metric >> PRECISIONSHIFT;
	const int16_t max =
	        ((RATE * ((256 - 1) >> METRICSHIFT)) >> PRECISIONSHIFT);
	  
	m0 = vp -> old_metrics->t [i] + metric;
	m1 = vp -> old_metrics->t [i + NUMSTATES / 2] + (max - metric);
	m2 = vp -> old_metrics->t [i] + (max - metric);
	m3 = vp -> old_metrics->t [i + NUMSTATES / 2] + metric;
	  
	decision0 = ((int32_t)(m0 - m1)) > 0;
	decision1 = ((int32_t)(m2 - m3)) > 0;
	  
	vp -> new_metrics-> t[2 * i] = decision0 ? m1 : m0;
	vp -> new_metrics-> t[2 * i + 1] =  decision1 ? m3 : m2;
	  
	d -> w[i/(sizeof(uint32_t)*8/2)+s*(sizeof(decision_t)/sizeof(uint32_t))] |= 
		    (decision0|decision1<<1) << ((2*i)&(sizeof(uint32_t)*8-1));
}

/*
 * Update decoder with a block of demodulated symbols
 * Note that nbits is the number of decoded data bits, not the number
 * of symbols!
 */
void	viterbi::update_viterbi_blk_GENERIC (struct v *vp,
					    int16_t *syms, int16_t nbits){
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
/*
 *	Viterbi chainback
 */
void	viterbi::chainback_viterbi (struct v *vp,
	                            uint8_t *data, /* Decoded output data */
	                            int16_t nbits, /* Number of data bits */
	                            uint16_t endstate){ /*Terminal encoder state */
decision_t *d = vp -> decisions;

/*
 *	Make room beyond the end of the encoder register so we can
 *	accumulate a full byte of decoded data
 */
	endstate = (endstate % NUMSTATES) << ADDSHIFT;
/*
 *	The store into data[] only needs to be done every 8 bits.
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
	vp -> old_metrics-> t [starting_state & (NUMSTATES-1)] = 0;
}

