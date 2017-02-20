/* Include file to configure the RS codec for character symbols
 *
 * Copyright 2002, Phil Karn, KA9Q
 * May be used under the terms of the GNU General Public License (GPL)
 */

#ifndef	__REED_SOLOMON
#define	__REED_SOLOMON

#include	<stdint.h>
#include	"galois.h"

class	reedSolomon {
private:
	galois	myGalois;
	uint16_t symsize;		/* Bits per symbol */
	uint16_t codeLength;		/* Symbols per block (= (1<<mm)-1) */
	uint8_t *generator;	/* Generator polynomial */
	uint16_t nroots;	/* Number of generator roots = number of parity symbols */
	uint8_t fcr;		/* First consecutive root, index form */
	uint8_t prim;		/* Primitive element, index form */
	uint8_t iprim;		/* prim-th root of 1, index form */
	bool	computeSyndromes	(uint8_t *, uint8_t *);
	uint8_t	getSyndrome		(uint8_t *, uint8_t);
	uint16_t computeLambda		(uint8_t *, uint8_t *);
	int16_t	computeErrors		(uint8_t *, uint16_t,
	                                 uint8_t *, uint8_t *);
	uint16_t computeOmega		(uint8_t *, uint8_t *, uint16_t, uint8_t *);
	void	encode_rs		(const uint8_t *data_in,
	                                      uint8_t *roots);
	int16_t	decode_rs		(uint8_t *data);
public:
		reedSolomon (uint16_t symsize	= 8,
	                     uint16_t gfpoly	= 0435,
	                     uint16_t fcr	= 0,
	                     uint16_t prim	= 1,
	                     uint16_t nroots	= 10);
		~reedSolomon (void);
int16_t		dec	  (const uint8_t *data_in, uint8_t *data_out, int16_t cutlen);
void		enc	  (const uint8_t *data_in, uint8_t *data_out, int16_t cutlen);
};

#endif
