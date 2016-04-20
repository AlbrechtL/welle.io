#
/*
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
 *    SDR-J is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with SDR-J; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#
#include	<stdio.h>
#include	<stdint.h>
#include	"reedsolomon.h"

#define	CODE_LENGTH	((1 << 8) - 1)
	
static inline
int	modnn (int x){
	while (x >= (1 << 8) - 1) {
	   x -= (1 << 8) - 1;
	   x = (x >> 8) + (x & ((1 << 8) - 1));
	}
	return x;
}

	reedSolomon::reedSolomon (uint16_t gfpoly,
	                          uint16_t fcr,
	                          uint16_t prim,
	                          uint16_t nroots):myGalois (gfpoly) {
uint16_t v1 [2], v2 [2], *res1;
int16_t i;

	this	-> gfpoly	= gfpoly;
	this	-> fcr	= fcr;
	this	-> prim	= prim;
	this	-> nroots	= nroots;
	generator	= new uint16_t [nroots + 1];
	syndromes	= new uint16_t [nroots];
	termPoly [0]	= 1;
	termPoly [1]	= prim;	// first root generator polynome
//	lousy code for the generator polynome
	v1 [0] = 1; v1 [1] = termPoly [1];
	v2 [0] = 1; v2 [1] = 2;
	
	res1	= multiply (v1, v2, 1, 1);
	for (i = 2; i < nroots; i ++) {
	   v2 [1] = myGalois. multiply_poly (2,  v2 [1]);
	   res1	= multiply (v2, res1, 1, i);
	}
	for (i = 0; i < nroots + 1; i ++)
	   generator [i] = res1 [i]; 
}

	reedSolomon::~reedSolomon (void) {
	delete [] generator;
	delete [] syndromes;
}

void	reedSolomon::encode (const uint8_t *inp,
	                     uint8_t * u, uint16_t f_cut) {
int16_t	i;
uint16_t temp [CODE_LENGTH];
uint16_t *p = &temp [0];
int16_t	o1	= CODE_LENGTH - 1, o2	= nroots;
//
//	we use "temp" as intermediate. We start by multiplying
//	it by x^nroots by simply shifting it nroots places to the left
//	(and making the order 255 by adding nroots zeros at the right)
	for (i = 0; i < f_cut; i ++) {
	   temp [i]	= 0;
	}
	for (i = 0; i < CODE_LENGTH - nroots - f_cut; i ++) {
	   u [i] = inp [i];
	   temp [f_cut + i] = inp [i];
	}
//
//	Initialize the parity bytes
	for (i = CODE_LENGTH - nroots; i < CODE_LENGTH; i ++)
	   temp [i] = 0;

//	as long as o1 >= o2, p can still be divided by the generator
	while (o1 >= o2) {
	   int16_t j, d;
//	we know the highest order term of generator = 1, so we
//	subtract p [0] * generator [j]	
	   d	= p [0];	// equals p [0] / generator [0]
	   if (d != 0) {
	      for (j = 0; j <= o2; j ++) 
	         p [j] =  myGalois. add_poly (p [j],
	                        myGalois. multiply_poly (d,  generator [j]));
	   }
//
//	and shift out the most left digit of the nominator,
//	reducing the order
	   o1 -= 1;
	   p ++;
	}

	for (i = 0; i < nroots; i ++) {
	   u [CODE_LENGTH - nroots - f_cut + i] = p [i];
	}
}
//
//	function returns number of errors if repair was possible
//	returns 0 if no errors detected
//	returns 1 if errors beyond being correctable
int16_t	reedSolomon::decode (const uint8_t *data_in,
	                     uint8_t *data_out, uint16_t f_cut) {
uint16_t werk [CODE_LENGTH];
int16_t i;
int16_t	result;
	for (i = 0; i < f_cut; i ++)
	   werk [i] = 0;
	for (i = 0; i < CODE_LENGTH - f_cut; i ++) 
	   werk [f_cut + i] = data_in [i];

	result = dec (werk);
	for (i = 0; i < CODE_LENGTH - f_cut - nroots; i ++)
	   data_out [i] = werk [f_cut + i];
	return result;
}
//
//	This is the big one, we go step by step
int16_t	reedSolomon::dec (uint16_t *werk) {
int16_t	i, j;
uint16_t deg_lambda, deg_omega;
uint16_t Lambda [nroots + 1];
uint16_t Omega  [nroots + 1];
uint16_t rootTable [nroots];
uint16_t locTable  [nroots];
int16_t rootCount;

//	Step 1: compute syndromes 
	if (computeSyndromes (syndromes, werk) == 0)
	   return 0;

//	fprintf (stderr, "syndromes ");
//	for (i = 0; i < nroots; i ++)
//	   fprintf (stderr, " %d (%d)",
//	                 syndromes [i], myGalois. poly2power (syndromes [i]));
//	fprintf (stderr, "\n");
//	Step 2: Berlekamp-Massey
	deg_lambda = computeBerlekamp (syndromes, Lambda);

//	fprintf (stderr, "Lambda ");
//	for (i = 0; i < nroots; i ++)
//	   fprintf (stderr, " %d", myGalois. power2poly (Lambda [i]));
//	fprintf (stderr, "\n");
//	Step 3:
//	Compute the error locations
	rootCount = computeErrors (Lambda, deg_lambda, rootTable, locTable);
	if (rootCount < 0)
	   return rootCount;

//	Step 4:
//	Compute Omega
	deg_omega = computeOmega (Lambda,
	                          deg_lambda,
	                          Omega,
	                          rootTable,
	                          rootCount);
/*
 *	Compute error values in poly-form.
 *	num1 = omega (inv (X (l))),
 *	num2 = inv (X (l))**(FCR-1) and
 *	den = lambda_pr(inv(X(l))) all in poly-form
 */
	uint16_t num1, num2, den;

	for (j = rootCount - 1; j >= 0; j--) {
	   num1 = 0;
	   for (i = deg_omega; i >= 0; i--) {
	      if (Omega [i] != 0) {
	         uint16_t aa1 = modnn (i * rootTable [j] + 1);
	         uint16_t aa2 = myGalois. multiply_power (Omega [i], aa1);
	         uint16_t aa3 = myGalois. power2poly (aa2);
	         uint16_t xx1 = modnn (Omega [i] + i * rootTable [j]);
	         uint16_t xx2 = myGalois. power2poly (xx1);
	         num1 = myGalois.  add_poly (num1, xx2);
	      }
	   }

//	here we need a compensation (i.e. increasing the power by 1)
	   num2 = myGalois. power2poly (modnn (rootTable [j] *
	                                         (fcr - 1) + (1 << 8) - 1 + 1));
	   den = 0;
/*
 *	lambda [i + 1] for i even is the formal derivative
 *	lambda_pr of lambda[i]
 */
	   i = deg_lambda < nroots ? deg_lambda & ~1 : nroots & ~1;
	   for (; i >= 0; i -=2) {
	      if (Lambda [i + 1] != 0)
	         den =  myGalois. add_poly (den, 
	                     myGalois. power2poly (
	                           modnn (Lambda [i + 1] +
	                                  i * rootTable [j] + 1)));
	   }

	   if (den == 0) {
	      fprintf (stderr, "den = 0, (count was %d)\n", den);
	      return -1;
	   }

//	   fprintf (stderr, "num = %d, num2 = %d, den = %d\n",
//	            num1, num2, den);
/*	Apply error to data */
	   if (num1 != 0) {
	      if ((locTable [j] < 135) ||
	          (locTable [j] >= 245)) {
	         rootCount --;
	         continue;
	      }
	      uint16_t corr = myGalois. power2poly (modnn (
	                                       myGalois. poly2power (num1) +
	                                       myGalois. poly2power (num2) +
	                                       CODE_LENGTH -
	                                       myGalois. poly2power (den)));
	      werk [locTable [j]] = myGalois. add_poly (werk [locTable [j]],
	                                                corr);
 	   }

 	}
	return rootCount;
}

uint16_t reedSolomon::computeSyndromes (uint16_t *syndromes, uint16_t *werk) {
int16_t o1	= CODE_LENGTH - 1;
int16_t i, j;
uint16_t v2 [2];
uint16_t result	= 0;

	v2 [0] = 1;
	v2 [1] = termPoly [1];		// first root
//
//	Simple approach in computing the syndrome: just compute
//	the remainder(s) from the inputword with the terms of the
//	generator polynome
	for (i = 0; i < nroots; i ++) {
	   o1	= CODE_LENGTH - 1;
	   uint16_t temp [o1 + 1];
	   uint16_t *p = &temp [0];
	   memcpy (temp, werk, (o1 + 1) * sizeof (uint16_t));
//	   as long as o1 >= 1, we keep on
	   while (o1 >= 1) {
//	we now shifted the denominator to the left
//	      int16_t d	= myGalois.  divide_poly (p [0], v2 [0]);
	      uint16_t d	= p [0];
	      if (d != 0) {
	         for (j = 0; j <= 1; j ++) 
	            p [j] =  myGalois. add_poly (p [j],
	                        myGalois. multiply_poly (d,  v2 [j]));
	      }
//
//	and shift out the most left digit of the nominator,
//	reducing the order
	      o1 -= 1;
	      p ++;
	   }
	   syndromes [i] = p [0];
	   result |= p [0];
	   v2 [1] = myGalois.  multiply_poly (2,  v2 [1]);
	}
	return result;
}

/*
 *	Begin Berlekamp-Massey algorithm to determine error
 *	locator polynomial.
 *	This is a direct implementation of the algorithm as
 *	presented in WHP031
 *
 *	functions returns degree of lambda
 */
int16_t	reedSolomon::computeBerlekamp (uint16_t * syndromes, uint16_t *Lambda) {
uint16_t K = 1, L = 0;
uint16_t Corrector	[nroots + 1];
int16_t  i;
int16_t	deg_lambda;

	for (i = 0; i < nroots + 1; i ++)
	   Corrector [i] = Lambda [i] = 0;
	uint16_t	error	= syndromes [0];
//
//	Initializers: 
	Lambda	[0]	= 1;
	Corrector [1]	= 1;
//
//
	while (K <= nroots) {
	   uint16_t oldLambda [nroots + 1];
	   memcpy (oldLambda, Lambda, (nroots + 1) * sizeof (uint16_t));
//
//	Compute new lambda
	   for (i = 0; i < nroots + 1; i ++)
	      Lambda [i] = myGalois. add_poly (Lambda [i],
	                               myGalois. multiply_poly (error, 
	                                                   Corrector [i]));
	   if ((2 * L < K) && (error != 0)) {
	      L = K - L;
	      for (i = 0; i < nroots + 1; i ++)
	         Corrector [i] = myGalois. divide_poly (oldLambda [i], error);
	   }
//
//	multiply x * C (x), i.e. shift to the right, the 0-th order term is left
	   for (i = nroots; i >= 1; i --)
	      Corrector [i] = Corrector [i - 1];
	   Corrector [0] = 0;

//	and compute a new error
	   error	= syndromes [K];	
	   for (i = 1; i <= K; i ++)  {
	      error = myGalois. add_poly (error,
	                           myGalois. multiply_poly (syndromes [K - i],
	                                                      Lambda [i]));
	   }
	   K += 1;
 	} // end of Berlekamp loop

	for (i = 0; i < nroots + 1; i ++) {
	   Lambda [i] = myGalois. poly2power (Lambda [i]);
	   if (Lambda [i] != 0)
	      deg_lambda = i;
	}
	return deg_lambda;
}
//
//	Compute the error locations and return the count

int16_t	 reedSolomon::computeErrors (uint16_t *Lambda,
	                             uint16_t deg_lambda,
	                             uint16_t *rootTable,
	                             uint16_t *locTable) {
int16_t i, j, k;
int16_t rootCount = 0;
//	for all powers we compute the lambda form with the
//	x replaced by alpha^i
	for (i = 1, k = 0; i <= CODE_LENGTH; i ++, k ++) 
	   if (compute (Lambda, deg_lambda, i) == 0) {
//
//	store root and error location number
	      locTable [rootCount] = k;
	      rootTable [rootCount ++] = i;
	   }
//	copy of lambda
//uint16_t reg [nroots + 1];
//	memcpy (&reg, Lambda, (nroots + 1) * sizeof (uint16_t));
//	
//	for (i = 1, k = 0; i <= CODE_LENGTH; i ++, k ++) {
//	   uint16_t result = 1;	// lambda [0] is always 0
//	   for (j = deg_lambda; j > 0; j --) {
//	      if (reg [j] != CODE_LENGTH) {
//	         reg [j] = myGalois. multiply_power (reg [j], j);
//	         result = myGalois. add_poly (result,
//	                                      myGalois. power2poly (reg [j]));
//	      }
//	   }
//	   if (result != 0)		// no root
//	      continue;
//	   fprintf (stderr, " %d %d", i, k);
//	   rootTable [rootCount] = i;
//	   locTable  [rootCount] = k;
//	   rootCount ++;
//	}
//	fprintf (stderr, "\n");
	if (rootCount != deg_lambda)
	   return -1;
	return rootCount;
}
//
//	Compute error evaluator poly omega
//	omega (x) = syndrome (x) * lambda (x) (modulo x ** NROOTS)
//	in index form, and return as function value the degree
int16_t	reedSolomon::computeOmega (uint16_t *Lambda,
	                           uint16_t deg_lambda,
	                           uint16_t *omega,
	                           uint16_t *rootTable,
	                           uint16_t rootCount) {
int16_t i, j;
uint16_t deg_omega	= 0;
	for (i = 0; i < nroots; i ++) {
	   uint16_t tmp = 0;
	   j = (deg_lambda < i) ? deg_lambda : i;
	   for (; j >= 0; j --) {
	      if ((syndromes [i - j] != 255) && (syndromes [i - j] != 0) &&
	          (Lambda [j] != 0))
	         tmp = myGalois. add_poly (tmp,
	                               myGalois.  multiply_poly (
	                                  syndromes [i - j],
	                                  myGalois. power2poly (Lambda [j])));
	      if (tmp != 0)
	         deg_omega = i;
	      omega [i] = myGalois. poly2power (tmp);
	   }
	}
//	fprintf (stderr, "Omega ");
//	for (i = 0; i < nroots; i ++)
//	   fprintf (stderr, " %d", myGalois. power2poly (omega [i]));
//	fprintf (stderr,"\n");
	
	omega [nroots] = 0;
	return deg_omega;
}

//	compute the function f (degree d) for a value
//	alpha^-p, i.e. the real work for Chien's search

uint16_t reedSolomon::compute (uint16_t *f, uint16_t d, uint16_t p) {
uint16_t i;
uint16_t power = myGalois. inverse_power (p + 1);
uint16_t x_term	= power;
//	first order term is simple
uint16_t sum = myGalois. add_power (f [d],
	                     myGalois. multiply_power (f [d - 1], x_term));
//
//	higher order terms
	for (i = 2; i <= d; i ++) {
	   x_term = myGalois. multiply_power (x_term, power);
	   sum = myGalois. add_power (sum,
	                               myGalois. multiply_power (x_term,
	                                                           f [d - i]));
	}

	return  sum;
}

uint16_t *reedSolomon::multiply (uint16_t *l1,
	                         uint16_t *l2,
	                         int16_t o1, int16_t o2) {
int16_t i, j;
uint16_t	*result	= new uint16_t [o2 + o1 + 1];

	memset (result, 0, (o2 + o1 + 1) * sizeof (int16_t));
	for (i = 0; i <= o2; i ++)		// 0-th order, just multiply
	   result [i + o1] = myGalois. multiply_poly (l1 [o1], l2 [i]);

	for (i = 1; i <= o1; i ++) {
	   int16_t temp [o2 + i];
	   for (j = 0; j <= o2; j ++)		// compute shifted l1 [i] * l2;
	      temp [j] = myGalois. multiply_poly (l1 [o1 - i], l2 [j]);
	   for (j = o2 + 1; j <= o2 + i; j ++)	// and append 0
	      temp [j] = 0;
	   for (j = 0; j <= o2 + i + 1; j ++)
	      result [o1 + o2 - j] = myGalois. add_poly (result [o1 + o2 - j],
	                                   temp [o2 + i - j]);
	}
	return result;
}

