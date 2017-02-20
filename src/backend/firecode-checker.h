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
#ifndef	FIRECODE_CHECKER
#define	FIRECODE_CHECKER
#include	<stdint.h>


class firecode_checker {
public:
		firecode_checker	(void);
		~firecode_checker	(void);
// error detection. x[0-1] contains parity, x[2-10] contains data
	bool	check (const uint8_t *x); // return true if firecode check is passed
private:
	uint16_t tab[256];
	uint16_t run8(unsigned char regs[]);
	static const uint8_t g[16];
};

#endif

