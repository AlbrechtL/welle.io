#
/*
 *
 *    Copyright (C) 2010, 2011, 2012
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the SDR-J.
 *    Many of the ideas as implemented in SDR-J are derived from
 *    other work, made available through the GNU general Public License. 
 *    All copyrights of the original authors are recognized.
 *
 *    SDR-J is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    JSDR is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with SDR-J; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef	__REEDSOLOMON
#define	__REEDSOLOMON

#include	<stdint.h>
#include	"galois.h"

class	reedSolomon {
public:
		reedSolomon (uint16_t gfpoly	= 0435,
	                     uint16_t fcr	= 0,
	                     uint16_t prim	= 1,
	                     uint16_t nroots	= 10);
		~reedSolomon (void);
void		encode	(const uint8_t *data_in,
	                 uint8_t *data_out, uint16_t f_cut);
int16_t		decode	(const uint8_t *data_in,
	                 uint8_t *data_out, uint16_t f_cut);
private:
	galois		myGalois;
	int16_t		dec			(uint16_t *);
	uint16_t	computeSyndromes	(uint16_t *, uint16_t *);
	int16_t		computeBerlekamp	(uint16_t *, uint16_t *);
	int16_t		computeErrors		(uint16_t *,
	                                         uint16_t,
	                                         uint16_t *,
	                                         uint16_t *);
	int16_t		computeOmega		(uint16_t *,
	                                         uint16_t,
	                                         uint16_t *,
	                                         uint16_t *,
	                                         uint16_t);
	uint16_t	compute			(uint16_t *f,
	                                         uint16_t, uint16_t);
	uint16_t	*multiply		(uint16_t *,
	                                         uint16_t *,
	                                         int16_t, int16_t);
	uint16_t	gfpoly;
	uint16_t	fcr;
	uint16_t	prim;
	uint16_t	nroots;
	uint16_t	*generator;
	uint16_t	*syndromes;
	uint16_t 	termPoly [2];
};
#endif

