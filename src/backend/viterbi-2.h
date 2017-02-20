#
/*
 *
 *    Copyright (C) 2013
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
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
 */
#
/*
 *    viterbi.h  --  Viterbi decoder
 *
 */
#ifndef _VITERBI_DECODER_H
#define _VITERBI_DECODER_H
#include	<stdio.h>
#include	<string.h>
#include	<limits.h>
#include	<stdint.h>

//
//	experimental setting for 1/4 7 decoder
class viterbi_2 {
public:
		viterbi_2			(int16_t);
		~viterbi_2 		(void);

	void	deconvolve		(int16_t *, uint8_t *);
private:
	int16_t	costsFor		(uint16_t, int16_t *);
//	int16_t	costsFor		(uint16_t, uint8_t, int16_t *);
	uint8_t	bitFor			(uint16_t, uint16_t, uint8_t);
	int16_t	Poly1;
	int16_t	Poly2;
	int16_t	Poly3;
	int16_t	Poly4;
	int32_t	**transCosts;
	int16_t	**history;
	int16_t	*sequence;
	uint8_t	*poly1_table;
	uint8_t	*poly2_table;
	uint8_t	*poly3_table;
	uint8_t	*poly4_table;
	uint8_t	*table5;
	int16_t	blockLength;
};
#endif
