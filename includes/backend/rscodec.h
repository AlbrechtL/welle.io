#
/* -*- c++ -*- */
/*
 * Copyright 2004,2010 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

//
//	In the SDR-J DAB+ software, we use a - slighty altered -
//	version of the dabp_rscodec as found in GnuRadio.
//	For the Galois fields, we use a different implementation
#ifndef	RSCODEC
#define	RSCODEC

#include	<stdint.h>
#include	<stdint.h>
#include	<cstring>


#define	GALOIS_DEGREE	8
#define	ERROR_CORRECT	5
#define	START_J		0

class rscodec {
public:
	// GF(2^m), error correction capability t,
	// start_j is the starting exponent in the generator polynomial
	rscodec (void);
	 ~rscodec ();
// decode shortened code
	int16_t dec (const uint8_t *r, uint8_t *d, int16_t cutlen = 135);
//	encode shortened code, cutlen bytes were shortened
//	not used for the DAB+ decoding
	void enc (const uint8_t *u, uint8_t *c, int16_t cutlen = 135);
private:
	static	const int16_t d_m	= 8;		// m
	static	const int16_t d_q	= 1 << 8;	// q = 2 ^ m
// primitive polynomial to generate GF(q)
	static	const int16_t d_p	= 0435;		
// starting exponent for generator polynomial, j
	static	const int16_t	d_j	= 0;
//
//	LUT translating power form to polynomial form
	int gexp [512];
//	LUT translating polynomial form to power form
	int glog [512];
	
// all in power representations
 	int16_t	add_poly	(int16_t a, int16_t b);
	int16_t add_power	(int16_t a, int16_t b);
	int16_t multiply_poly	(int16_t a, int16_t b); // a*b
	int16_t multiply_power	(int16_t a, int16_t b);
	int16_t divide_poly	(int16_t a, int16_t b); 	// a/b
	int16_t divide_power	(int16_t a, int16_t b);
	int16_t pow_poly	(int16_t a, int16_t n);		// a^n
	int16_t pow_power	(int16_t a, int16_t n);
	int16_t power2poly	(int16_t a);
	int16_t poly2power	(int16_t a);
	int16_t	inverse_poly	(int16_t a);
	int16_t	inverse_power	(int16_t a);

// convert a polynomial representation to m-tuple representation
// returned in tuple, lowest degree first.
// tuple must have size d_m at minimum
	void poly2tuple		(int16_t a, uint8_t tuple[]);
	void power2tuple	(int16_t a, uint8_t tuple[]);

// round mod algorithm, calculate a % n, n > 0, a is any integer, a % n >= 0
	int16_t round_mod (int16_t a, int16_t n);

// u and c are in polynomial representation. This is more common in practice
	void enc_poly (const uint16_t * u, uint16_t * c);
	   
// r and d are in polynomial representation. This is more common in practice
	int16_t dec_poly (const uint16_t *r, uint16_t *d);

//	int d_m;		// GF(2^m)
//	dabp_galois d_gf;	// Galois field

// d_g[0] is the lowest exponent coefficient,
// d_g[2t-1] is the highest exponent coefficient, d_g[2t]=1 is not included
	int *d_g;		// generator g.

// error correcting capability t, info length k, codeword length n,
// all measured in unit of GF(2^m) symbols
	
	int *d_reg;	// registers for encoding
	int *syndrome;	// syndrome
	int *d_euc [2];	// data structure for Euclidean computation
	
	void create_polynomials (int start_j); // initialize the generator polynomial g
};

#endif		// DABP_RSCODE

