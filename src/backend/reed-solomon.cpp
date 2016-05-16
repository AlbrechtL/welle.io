#
/* Initialize a RS codec
 *
 * Copyright 2002 Phil Karn, KA9Q
 * May be used under the terms of the GNU General Public License (GPL)
 */
#include	<stdio.h>
#include	"reed-solomon.h"
#include	<string.h>

/*
 *	Reed-Solomon decoder
 *	Copyright 2002 Phil Karn, KA9Q
 *	May be used under the terms of the GNU General Public License (GPL)
 */
/*
 *	Rewritten - and slightly adapted while doing so -
 *	as a C++ class for use in the sdr-j dab decoder(s)
 *	Copyright 2015 Jan van Katwijk
 *	May be used under the terms of the GNU General Public License (GPL)
 */
#define	min(a,b)	((a) < (b) ? (a) : (b))

/* Initialize a Reed-Solomon codec
 * symsize = symbol size, bits (1-8)
 * gfpoly = Field generator polynomial coefficients
 * fcr = first root of RS code generator polynomial, index form, 0
 * prim = primitive element to generate polynomial roots
 * nroots = RS code generator polynomial degree (number of roots)
 */

	reedSolomon::reedSolomon (uint16_t symsize,
	                          uint16_t gfpoly,
	                          uint16_t fcr,
	                          uint16_t prim,
	                          uint16_t nroots):
	                            myGalois (symsize, gfpoly) {
int i, j, root, iprim;

	this	-> symsize 	= symsize;		// in bits
	this	-> codeLength	= (1 << symsize) - 1;
	this	-> fcr		= fcr;
	this	-> prim		= prim;
	this	-> nroots	= nroots;
	for (iprim = 1; (iprim % prim) != 0; iprim += codeLength);
	this -> iprim = iprim / prim;
	this	-> generator	= new uint8_t [nroots + 1];
	memset (generator, 0, (nroots + 1) * sizeof (generator [0]));
	generator [0] = 1;

	for (i = 0, root = fcr * prim; i < nroots; i++, root += 1) {
	   generator [i + 1] = 1;
	   for (j = i; j > 0; j--){
	      if (generator [j] != 0) {
	         uint16_t p1 = myGalois. multiply_power (
	                                   myGalois. poly2power (generator [j]),
	                                   root);
	         generator [j] = myGalois. add_poly (
	                generator [j - 1],
	                myGalois. power2poly (p1));
	         
	      }
	      else {
	         generator [j] = generator [j - 1];
	      }
	   }

/*	rsHandle -> genpoly [0] can never be zero */
	   generator [0] =
	           myGalois. power2poly (
	                myGalois. multiply_power (root,
	                          myGalois. poly2power (generator [0])));
	}
	for (i = 0; i <= nroots; i ++)
	   generator [i] = myGalois. poly2power (generator [i]);
}

	reedSolomon::~reedSolomon	(void) {
	delete generator;
}

//
//	Basic encoder, returns - in bb - the parity bytes
void	reedSolomon::encode_rs (const uint8_t *data, uint8_t *bb){
int i, j;
uint8_t feedback;

	memset (bb, 0, nroots * sizeof (uint8_t));

	for (i = 0; i < codeLength - nroots; i++){
	   feedback = myGalois. poly2power (
	                       myGalois. add_poly (data [i], bb [0]));
	   if (feedback != codeLength){ /* feedback term is non-zero */
	      for (j = 1; j < nroots; j++)
	         bb [j] = myGalois. add_poly (bb [j],
	                      myGalois. power2poly (
	                         myGalois. multiply_power (feedback,
	                                   generator [nroots - j])));
	   }
/*	Shift */
	   memmove (&bb [0], &bb[1], sizeof (bb [0]) * (nroots - 1));
	   if (feedback != codeLength)
	      bb [nroots - 1] =
	          myGalois. power2poly (
	                 myGalois. multiply_power (feedback,
	                                           generator [0]));
	   else
	      bb [nroots - 1] = 0;
	}
}

void	reedSolomon::enc (const uint8_t *r, uint8_t *d, int16_t cutlen) {
uint8_t rf [codeLength];
uint8_t bb [nroots];
int16_t i;

	memset (rf, 0, cutlen * sizeof (rf [0]));
	for (i = cutlen; i < codeLength; i++)
	   rf [i] = r[i - cutlen];

	encode_rs (rf, bb);
	for (i = cutlen; i < codeLength - nroots; i++)
	   d [i - cutlen] = rf [i];
//	and the parity bytes
	for (i = 0; i < nroots; i ++)
	   d [codeLength - cutlen - nroots + i] = bb [i];
}


int16_t	reedSolomon::dec (const uint8_t *r, uint8_t *d, int16_t cutlen) {
uint8_t rf [codeLength];
int16_t i;
int16_t	ret;

	memset (rf, 0, cutlen * sizeof (rf [0]));
	for (i = cutlen; i < codeLength; i++)
	   rf [i] = r [i - cutlen];

	ret = decode_rs (rf);
	for (i = cutlen; i < codeLength - nroots; i++)
	   d [i - cutlen] = rf [i];
	return ret;
}

int16_t	reedSolomon::decode_rs (uint8_t *data) {
uint8_t syndromes [nroots];
uint8_t Lambda	  [nroots + 1];
uint16_t lambda_degree, omega_degree;
uint8_t	rootTable [nroots];
uint8_t	locTable  [nroots];
uint8_t	omega	  [nroots + 1];
int16_t	rootCount;
int16_t	i;
//
//	returning syndromes in poly
	if (computeSyndromes (data, syndromes))
	   return 0;
//	Step 2: Berlekamp-Massey
//	Lambda in power notation
	lambda_degree = computeLambda (syndromes, Lambda);

//	Step 3: evaluate lambda and compute the error locations (chien)
	rootCount = computeErrors (Lambda, lambda_degree, rootTable, locTable);
	if (rootCount < 0)
	   return -1;
	omega_degree = computeOmega (syndromes, Lambda, lambda_degree, omega);
/*
 *	Compute error values in poly-form.
 *	num1 = omega (inv (X (l))),
 *	num2 = inv (X (l))**(FCR-1) and
 *	den = lambda_pr(inv(X(l))) all in poly-form
 */
	uint16_t num1, num2, den;
	int16_t j;
	for (j = rootCount - 1; j >= 0; j--) {
	   num1 = 0;
	   for (i = omega_degree; i >= 0; i--) {
	      if (omega [i] != codeLength) {
	         uint16_t tmp = myGalois. multiply_power (omega [i],
	                           myGalois. pow_power (i, rootTable [j]));
	         num1	= myGalois. add_poly (num1, 
	                              myGalois. power2poly (tmp));
	      }
	   }
	   uint16_t tmp = myGalois. multiply_power (
	                              myGalois. pow_power (
	                                     rootTable [j],
	                                     myGalois. divide_power (fcr, 1)),
	                              codeLength);
	   num2	= myGalois. power2poly (tmp);
	   den = 0;
/*
 *	lambda [i + 1] for i even is the formal derivative
 *	lambda_pr of lambda [i]
 */
	   for (i = min (lambda_degree, nroots - 1) & ~1;
	                 i >= 0; i -=2) {
	      if (Lambda [i + 1] != codeLength) {
	         uint16_t tmp = myGalois. multiply_power (Lambda [i + 1],
	                             myGalois. pow_power (i, rootTable [j]));
	         den	= myGalois. add_poly (den, myGalois. power2poly (tmp));
	      }
	   }

	   if (den == 0) {
//	      fprintf (stderr, "den = 0, (count was %d)\n", den);
	      return -1;
	   }
/*	Apply error to data */
	   if (num1 != 0) {
	      if (locTable [j] >=  uint8_t (codeLength - nroots))
	         rootCount --;
	      else {
	         uint16_t tmp1	= codeLength - myGalois. poly2power (den);
	         uint16_t tmp2	= myGalois. multiply_power (
	                               myGalois. poly2power (num1),
	                               myGalois. poly2power (num2));
	         tmp2		= myGalois. multiply_power (tmp2, tmp1);
	         uint16_t corr	= myGalois. power2poly (tmp2);
	         data [locTable [j]] =
	                         myGalois. add_poly (data [locTable [j]], corr);
	      }
 	   }
	
 	}
	return rootCount;
}
//
//	Apply Horner on the input for root "root"
uint8_t	reedSolomon::getSyndrome (uint8_t *data, uint8_t root) {
uint8_t	syn	= data [0];
int16_t j;

	for (j = 1; j < codeLength; j++){
	   if (syn == 0)
	      syn = data [j];
	   else {
	      uint16_t uu1 = myGalois. pow_power (
	                           myGalois. multiply_power (fcr, root),
	                           prim);
	      syn = myGalois. add_poly (data [j],
	                      myGalois. power2poly (
	                           myGalois. multiply_power (
	                               myGalois.poly2power (syn), uu1)));
//	                                                   (fcr + root) * prim)));
	   }
	}
	return syn;
}

//
//	use Horner to compute the syndromes
bool	reedSolomon::computeSyndromes (uint8_t *data, uint8_t *syndromes) {
int16_t i;
uint16_t syn_error;

/* form the syndromes; i.e., evaluate data (x) at roots of g(x) */

	for (i = 0; i < nroots; i++) {
	   syndromes [i] = getSyndrome (data, i);
	   syn_error |= syndromes [i];
	}

	return syn_error == 0;
}
//
//	compute Lambda with Berlekamp-Massey
//	syndromes in poly-form in, Lambda in power form out
uint16_t reedSolomon::computeLambda (uint8_t *syndromes, uint8_t *Lambda) {
uint16_t K = 1, L = 0;
uint8_t Corrector	[nroots + 1];
int16_t  i;
int16_t	deg_lambda;

	for (i = 0; i < nroots + 1; i ++)
	   Corrector [i] = Lambda [i] = 0;

	uint8_t	error	= syndromes [0];
//
//	Initializers: 
	Lambda	[0]	= 1;
	Corrector [1]	= 1;
//
	while (K <= nroots) {
	   uint8_t oldLambda [nroots + 1];
	   memcpy (oldLambda, Lambda, (nroots + 1) * sizeof (Lambda [0]));
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
	   if (Lambda [i] != 0)
	      deg_lambda = i;
	   Lambda [i] = myGalois. poly2power (Lambda [i]);
	}
	return deg_lambda;
}
//
//	Compute the roots of lambda by evaluating the
//	lambda polynome for all (inverted) powers of the symbols
//	of the data (Chien search)
int16_t  reedSolomon::computeErrors (uint8_t *Lambda,
	                             uint16_t deg_lambda,
	                             uint8_t *rootTable,
	                             uint8_t *locTable) {
int16_t i, j, k;
int16_t rootCount = 0;
//
	uint8_t workRegister [nroots + 1];
	memcpy (&workRegister, Lambda, (nroots + 1) * sizeof (uint8_t));
//
//	reg is lambda in power notation
	for (i = 1, k = iprim - 1;
	     i <= codeLength; i ++, k = (k + iprim)) {
	   uint16_t result = 1;	// lambda [0] is always 1
//	Note that for i + 1, the powers in the workregister just need
//	to be increased by "j".
	   for (j = deg_lambda; j > 0; j --) {
	      if (workRegister [j] != codeLength)  {
	         workRegister [j] = myGalois. multiply_power (workRegister [j],
	                                                      j);
	         result = myGalois. add_poly (result,
	                                       myGalois. power2poly
	                                              (workRegister [j]));
	      }
	   }
	   if (result != 0)		// no root
	      continue;
	   rootTable [rootCount] = i;
	   locTable  [rootCount] = k;
	   rootCount ++;
	}
	if (rootCount != deg_lambda)
	   return -1;
	return rootCount;
}

/*
 *	Compute error evaluator poly
 *	omega(x) = s(x)*lambda(x) (modulo x**NROOTS)
 *	in power form, and  find degree (omega).
 *
 *	Note that syndromes are in poly form, while lambda in power form
 */
uint16_t reedSolomon::computeOmega (uint8_t *syndromes,
	                            uint8_t *lambda, uint16_t deg_lambda,
	                            uint8_t *omega) {
int16_t i, j;
int16_t	deg_omega = 0;

	for (i = 0; i < nroots; i++){
	   uint16_t tmp = 0;
	   j = (deg_lambda < i) ? deg_lambda : i;
	   for (; j >= 0; j--){
	      if ((myGalois. poly2power (syndromes [i - j]) != codeLength) &&
	          (lambda [j] != codeLength)) {
	         uint16_t res = myGalois. power2poly (
	                             myGalois. multiply_power (
	                                           myGalois. poly2power (
	                                                 syndromes [i - j]), 
	                                           lambda [j]));
	         tmp =  myGalois. add_poly (tmp, res);
	      }
	   }

	   if (tmp != 0)
	      deg_omega = i;
	   omega [i] = myGalois. poly2power (tmp);
	}

	omega [nroots] = codeLength;
	return deg_omega;
}

