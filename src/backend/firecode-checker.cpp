#
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
//	This is a (partial) rewrite of the GNU radio code, for use
//	within the DAB/DAB+ sdr-j receiver software
//	all rights are acknowledged.
//
#include "firecode-checker.h"
#include <cstring>

//	g(x)=(x^11+1)(x^5+x^3+x^2+x+1)=1+x+x^2+x^3+x^5+x^11+x^12+x^13+x^14+x^16
const uint8_t firecode_checker::g[16]={1,1,1,1,0,1,0,0,0,0,0,1,1,1,1,0};

	firecode_checker::firecode_checker (void) {
// prepare the table
uint8_t regs [16];
int16_t i,j;
uint16_t itab [8];

	for (i = 0; i < 8; i++) {
	   memset (regs, 0, 16);
	   regs [8 + i] = 1;
	   itab [i] = run8 (regs);
	}
	for (i = 0; i < 256; i++) {
	   tab [i] = 0;
	   for (j = 0; j < 8; j++) {
	      if (i & (1 << j))
	         tab [i] = tab[i] ^ itab[j];
	   }
	}
}

	firecode_checker::~firecode_checker (void) {
}

uint16_t firecode_checker::run8 (uint8_t regs[]) {
int16_t i,j;
uint16_t z;
uint16_t v = 0;

	for (i = 0; i < 8; i++) {
	   z = regs [15];
	   for (j = 15; j > 0; j--)
	      regs [j] = regs [j-1] ^ (z & g[j]);
	   regs [0] = z;
	}

	for (i = 15; i >= 0; i--)
	   v = (v << 1) | regs[i];

	return v;
}

bool	firecode_checker::check (const uint8_t *x) {
int16_t i;
uint16_t state = (x[2] << 8) | x[3];
uint16_t istate;

	for (i = 4; i < 11; i++) {
	   istate = tab [state >> 8];
	   state = ((istate & 0x00ff) ^ x[i]) |
	           ((istate ^ state << 8) & 0xff00);
	}

	for (i = 0; i < 2; i++) {
	   istate = tab [state >> 8];
	   state = ((istate & 0x00ff) ^ x [i]) |
	           ((istate ^ state << 8) & 0xff00);
	}

	return state == 0;
}

