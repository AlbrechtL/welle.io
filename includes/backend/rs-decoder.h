/* Include file to configure the RS codec for character symbols
 *
 * Copyright 2002, Phil Karn, KA9Q
 * May be used under the terms of the GNU General Public License (GPL)
 */

#ifndef	__NEW_RSDECODER__
#define	__NEW_RSDECODER__

#include	<stdint.h>

typedef  uint8_t	DTYPE;

/* Reed-Solomon codec control block */
typedef struct rs {
	uint16_t mm;		/* Bits per symbol */
	uint16_t nn;		/* Symbols per block (= (1<<mm)-1) */
	uint8_t *alpha_to;	/* log lookup table */
	uint8_t *index_of;	/* Antilog lookup table */
	uint8_t *genpoly;	/* Generator polynomial */
	uint16_t nroots;	/* Number of generator roots = number of parity symbols */
	uint8_t fcr;		/* First consecutive root, index form */
	uint8_t prim;		/* Primitive element, index form */
	uint8_t iprim;		/* prim-th root of 1, index form */
} rsType;

static inline
int	modnn (struct rs *rs, int x){
	while (x >= rs->nn) {
	   x -= rs->nn;
	   x = (x >> rs->mm) + (x & rs->nn);
	}
	return x;
}

class	rsDecoder {
private:
	rsType	*rsHandle;
	bool	computeSyndromes	(DTYPE *, DTYPE *);
	uint16_t computeLambda		(DTYPE *, DTYPE *);
	uint16_t computeChien		(DTYPE *, int16_t, DTYPE *, DTYPE *);
	uint16_t computeOmega		(DTYPE *, DTYPE *, uint16_t, DTYPE *);
public:
		rsDecoder (uint16_t symsize,
	                   uint16_t gfpoly,
	                   uint16_t fcr,
	                   uint16_t prim,
	                   uint16_t nroots);
		~rsDecoder (void);
int16_t		decode_rs (DTYPE *data);
int16_t		dec	  (const DTYPE *data_in, DTYPE *data_out, int16_t cutlen);
void		enc	  (const DTYPE *data_in, DTYPE *data_out, int16_t cutlen);
void		encode	  (const DTYPE *data_in, DTYPE *roots);
};
#endif

