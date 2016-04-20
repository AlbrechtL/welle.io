/* Initialize a RS codec
 *
 * Copyright 2002 Phil Karn, KA9Q
 * May be used under the terms of the GNU General Public License (GPL)
 */
#include	<stdio.h>
#include	"rs-decoder.h"
#include	<string.h>

/* Reed-Solomon decoder
 * Copyright 2002 Phil Karn, KA9Q
 * May be used under the terms of the GNU General Public License (GPL)
 */
/*
 *	Rewritten as a C++ class for use in the 
 *	sdr-j dab decoder(s)
 */
#define	min(a,b)	((a) < (b) ? (a) : (b))

/* Initialize a Reed-Solomon codec
 * symsize = symbol size, bits (1-8)
 * gfpoly = Field generator polynomial coefficients
 * fcr = first root of RS code generator polynomial, index form
 * prim = primitive element to generate polynomial roots
 * nroots = RS code generator polynomial degree (number of roots)
 */

	rsDecoder::rsDecoder (uint16_t symsize,
	                      uint16_t gfpoly,
	                      uint16_t fcr,
	                      uint16_t prim,
	                      uint16_t nroots) {
int i, j, sr,root,iprim;

	rsHandle = new rsType;
	rsHandle -> mm = symsize;		// in bits
	rsHandle -> nn = (1 << symsize) - 1;

	rsHandle -> alpha_to = new DTYPE [rsHandle -> nn + 1];
	rsHandle -> index_of = new DTYPE [rsHandle -> nn + 1];
/*	Generate Galois field lookup tables */
	rsHandle -> index_of [0] = rsHandle -> nn; /* log (zero) = -inf */
	rsHandle -> alpha_to [rsHandle -> nn] = 0;	/* alpha**-inf = 0 */
	sr = 1;
	for (i = 0; i < rsHandle -> nn; i++){
	   rsHandle -> index_of [sr] = i;
	   rsHandle -> alpha_to [i] = sr;
	   sr <<= 1;
	   if (sr & (1 << symsize))
	      sr ^= gfpoly;
	   sr &= rsHandle -> nn;
	}

/*	Form RS code generator polynomial from its roots */
	rsHandle -> genpoly = new DTYPE [nroots + 1];
	rsHandle -> fcr = fcr;
	rsHandle -> prim = prim;
	rsHandle -> nroots = nroots;

/*	Find prim-th root of 1, used in decoding */
	for (iprim = 1; (iprim % prim) != 0; iprim += rsHandle -> nn) ;
	rsHandle -> iprim = iprim / prim;
	rsHandle -> genpoly [0] = 1;
	for (i = 0, root = fcr * prim; i < nroots; i++, root += prim) {
	   rsHandle -> genpoly [i + 1] = 1;

/*	Multiply rsHandle -> genpoly [] by  @**(root + x) */
	   for (j = i; j > 0; j--){
	      if (rsHandle -> genpoly [j] != 0)
	         rsHandle -> genpoly [j] = rsHandle -> genpoly [j - 1] ^
	            rsHandle -> alpha_to [modnn (rsHandle,
	                                         rsHandle -> index_of [rsHandle ->genpoly[j]] + root)];
	      else
	         rsHandle -> genpoly [j] = rsHandle -> genpoly [j - 1];
	   }

/*	rsHandle -> genpoly [0] can never be zero */
	   rsHandle -> genpoly [0] =
	            rsHandle -> alpha_to [modnn (rsHandle,
	                                         rsHandle -> index_of[rsHandle->genpoly[0]] + root)];
	}
/*	convert rsHandle -> genpoly [] to index form for quicker encoding */
	for (i = 0; i <= nroots; i++)
	   rsHandle -> genpoly [i] =
	              rsHandle -> index_of [rsHandle -> genpoly [i]];
}

	rsDecoder::~rsDecoder (void) {
	delete rsHandle -> alpha_to;
	delete rsHandle -> index_of;
	delete rsHandle -> genpoly;
	delete rsHandle;
}

int16_t	rsDecoder::decode_rs (DTYPE *data) {
int16_t deg_lambda, deg_omega;
int i, j, r,k;
DTYPE u,tmp,num1,num2,den;
DTYPE lambda [rsHandle -> nroots + 1],
      syndromes [rsHandle -> nroots];	/* Err Locator poly and syndrome poly */
DTYPE omega [rsHandle -> nroots + 1];

DTYPE root [rsHandle -> nroots],
	 loc [rsHandle -> nroots];
int16_t count;

	if (computeSyndromes (data, syndromes))	// no errors
	   return 0;
//
//	otherwise, there are errors!!
	deg_lambda = computeLambda (syndromes, lambda);

	count	= computeChien (lambda, deg_lambda, root, loc);
	if (count == -1)
	   return -1;

	deg_omega = computeOmega (syndromes, lambda, deg_lambda, omega);
/*
 *	Compute error values in poly-form.
 *	num1 = omega (inv (X (l))),
 *	num2 = inv (X (l))**(FCR-1) and
 *	den = lambda_pr(inv(X(l))) all in poly-form
 */
	for (j = count - 1; j >= 0; j--) {
	   num1 = 0;
	   for (i = deg_omega; i >= 0; i--) {
	      if (omega [i] != rsHandle -> nn) {
	         uint16_t xx1 = modnn (rsHandle, omega [i] + i * root[j]);
	         uint16_t xx2 = rsHandle -> alpha_to [modnn (rsHandle,
	                                               omega [i] + i * root[j])];
	         num1  ^= rsHandle -> alpha_to [modnn (rsHandle,
	                                               omega [i] + i * root[j])];
	      }
	   }

	   num2 = rsHandle -> alpha_to [modnn (rsHandle,
	                                       root [j] *
	                                       (rsHandle -> fcr - 1) + 
	                                       rsHandle -> nn)];
	   den = 0;
/*
 *	lambda [i + 1] for i even is the formal derivative
 *	lambda_pr of lambda [i]
 */
	   for (i = min (deg_lambda, rsHandle -> nroots - 1) & ~1;
	                 i >= 0; i -=2) {
	      if (lambda [i + 1] != rsHandle -> nn)
	         den ^= rsHandle -> alpha_to [modnn (rsHandle,
	                                             lambda [i + 1] + i * root [j])];
	   }

	   if (den == 0) {
	   fprintf (stderr, "den = 0, (count was %d)\n", den);
	      return -1;
	   }
/*	Apply error to data */
	   if (num1 != 0) {
	      if (loc [j] >=  (DTYPE)(rsHandle -> nn - rsHandle -> nroots))
	         count --;
	      uint16_t corr = rsHandle -> alpha_to [modnn (rsHandle,
	                                      rsHandle -> index_of [num1] +
	                                      rsHandle -> index_of [num2] + 
	                                      rsHandle -> nn -
	                                      rsHandle -> index_of [den])];
	      data [loc [j]] ^= corr;
//	      fprintf (stderr, "repairing loc %d (%d)\n", loc [j], corr);
 	   }
 	}
	return count;
}

bool	rsDecoder::computeSyndromes (DTYPE *data, DTYPE *syndromes) {
int16_t i, j;
uint16_t syn_error;

/* form the syndromes; i.e., evaluate data (x) at roots of g(x) */
	for (i = 0; i < rsHandle -> nroots; i++)
	   syndromes [i] = data [0];

	for (j = 1; j < rsHandle -> nn; j++){
	   for (i = 0; i < rsHandle -> nroots; i++){
	      if (syndromes [i] == 0){
	         syndromes [i] = data [j];
	      } else {
	         syndromes [i] = data [j] ^
	                     rsHandle -> alpha_to [modnn (rsHandle,
	                                                  rsHandle -> index_of [syndromes [i]] + (rsHandle -> fcr + i) * rsHandle -> prim)];
	      }
	   }
 	}
//	fprintf (stderr, "syndromes ");
//	for (i = 0; i < rsHandle -> nroots; i ++)
//	   fprintf (stderr, " %d", syndromes [i]);
//	fprintf (stderr, "\n");

/*	Convert syndromes to index form, checking for nonzero condition */
	syn_error = 0;
	for (i = 0; i < rsHandle -> nroots; i++){
	   syn_error |= syndromes [i];
	   syndromes [i] = rsHandle -> index_of [syndromes [i]];
	}

	return syn_error == 0;
}
//
//	Compte the Lambda and return the degree
uint16_t rsDecoder::computeLambda (DTYPE *syndromes, DTYPE *lambda) {
DTYPE b [rsHandle -> nroots + 1];
DTYPE t [rsHandle -> nroots + 1];
int	r, el, i;
uint16_t	deg_lambda;

	memset (&lambda [1], 0, rsHandle -> nroots * sizeof (lambda [0]));
	lambda [0] = 1;

	for (i = 0; i < rsHandle -> nroots + 1; i++)
	   b [i] = rsHandle -> index_of [lambda [i]];
  
/*
 *	Begin Berlekamp-Massey algorithm to determine error + erasure
 *	locator polynomial
 */
	r = 0;
	el = 0;
	while (++r <= rsHandle -> nroots) {	/* r is the step number */
/*	Compute discrepancy at the r-th step in poly-form */
	   int discr_r = 0;
	   for (i = 0; i < r; i++) {
	      if ((lambda [i] != 0) &&
	          (syndromes [r - i - 1] != rsHandle -> nn)) {
	         discr_r ^= rsHandle -> alpha_to [modnn (rsHandle,
	                                                 rsHandle -> index_of [lambda [i]] + syndromes [r - i - 1])];
	      }
	   }

	   discr_r = rsHandle -> index_of [discr_r];	/* Index form */
	   if (discr_r == rsHandle -> nn) {
/*	2 lines below: B(x) <-- x*B(x) */
	      memmove (&b [1], b, rsHandle -> nroots * sizeof (b[0]));
	      b [0] = rsHandle -> nn;
	   } else {
/*	7 lines below: T(x) <-- lambda(x) - discr_r*x*b(x) */
	      t [0] = lambda [0];
	      for (i = 0 ; i < rsHandle -> nroots; i++) {
	         if (b [i] != rsHandle -> nn)
	            t [i + 1] = lambda [i + 1] ^ 
	                            rsHandle -> alpha_to [modnn (rsHandle,
	                                                         discr_r + b [i])];
	         else
	            t [i + 1] = lambda [i + 1];
	      }

	      if (2 * el <= r - 1) {
	         el = r - el;
/*	2 lines below: B(x) <-- inv(discr_r) * lambda(x) */
	         for (i = 0; i <= rsHandle -> nroots; i++)
	            b[i] = (lambda[i] == 0) ?
	                   rsHandle -> nn :
	                   modnn (rsHandle,
	                          rsHandle -> index_of [lambda [i]] - discr_r + rsHandle -> nn);
	      } else {
/*	2 lines below: B(x) <-- x*B(x) */
	         memmove (&b [1], b, rsHandle -> nroots * sizeof (b [0]));
	         b [0] = rsHandle -> nn;
	      }

	      memcpy (lambda, t, (rsHandle -> nroots + 1) * sizeof (t[0]));
	   }
 	}	// end of berlekamp loop

//	fprintf (stderr, "lambda ");
//	for (i = 0; i < 10; i ++)
//	   fprintf (stderr, " %d", lambda [i]);
//	fprintf (stderr, "\n");
 /*	Convert lambda to index form and compute deg (lambda(x)) */
	deg_lambda = 0;
	for (i = 0; i < rsHandle -> nroots + 1; i++){
	   lambda [i] = rsHandle -> index_of [lambda [i]];
	   if (lambda [i] != rsHandle -> nn)
	      deg_lambda = i;
	}

	return deg_lambda;
}
//
//	do the chien search and return the number of roots found
//	and -1 is there is an inconsistency
uint16_t rsDecoder::computeChien (DTYPE *lambda, int16_t deg_lambda,
	                          DTYPE *root, DTYPE *loc) {
uint16_t i, j, k;
uint16_t q;
DTYPE reg [rsHandle -> nroots + 1];
uint16_t count;
/*	Find roots of the error locator polynomial by Chien search */
	memcpy (&reg [1], &lambda [1], rsHandle -> nroots * sizeof (reg [0]));
	count = 0;		/* Number of roots of lambda(x) */

	for (i = 1, k = rsHandle -> iprim - 1;
	     i <= rsHandle -> nn;
	     i++, k = modnn (rsHandle,  k + rsHandle -> iprim)) {
	   q = 1;	/* lambda [0] is always 0 */
	   for (j = deg_lambda; j > 0; j--){
	      if (reg [j] != rsHandle -> nn) {
	         reg [j] = modnn (rsHandle,  reg [j] + j);
	         q ^= rsHandle -> alpha_to [reg [j]];
	      }
	   }
	   if (q != 0)
	      continue; /* Not a root */

/*	store root (index-form) and error location number */
	   root [count] = i;
	   loc  [count] = k;
//	   fprintf (stderr, "root found %d (%d)\n", k, i);
	   ++count;
	}

//	deg (lambda) unequal to number of roots uncorrectable error detected
	if (deg_lambda != count) {
	   return -1;
  	}
	return count;
}

/*
 *	Compute error evaluator poly omega(x) = s(x)*lambda(x)
 *	(modulo	x**NROOTS) in index form. Also find deg (omega).
 */
uint16_t rsDecoder::computeOmega (DTYPE *syndromes,
	                         DTYPE *lambda, uint16_t deg_lambda,
	                         DTYPE *omega) {
int16_t i, j;
int16_t	deg_omega = 0;

	for (i = 0; i < rsHandle -> nroots; i++){
	   uint16_t tmp = 0;
	   j = (deg_lambda < i) ? deg_lambda : i;
	   for (; j >= 0; j--){
	      if ((syndromes [i - j] != rsHandle -> nn) &&
	          (lambda[j] != rsHandle -> nn))
	         tmp ^= rsHandle -> alpha_to [modnn (rsHandle,
	                                             syndromes [i - j] + lambda [j])];
	   }

	   if (tmp != 0)
	      deg_omega = i;
	   omega [i] = rsHandle -> index_of [tmp];
	}

//	fprintf (stderr, "omega ");
//	for (i = 0; i < rsHandle -> nroots; i ++)
//	   fprintf (stderr, " %d", rsHandle -> alpha_to [omega [i]]);
//	fprintf (stderr, "\n");
	omega [rsHandle -> nroots] = rsHandle -> nn;
	return deg_omega;
}

int16_t	rsDecoder::dec (const DTYPE *r, DTYPE *d, int16_t cutlen) {
DTYPE rf [rsHandle -> nn];
int16_t i;
int16_t	ret;

	memset (rf, 0, cutlen * sizeof (DTYPE));
	for (i = cutlen; i < rsHandle -> nn; i++)
	   rf [i] = r[i - cutlen];

	ret = decode_rs (rf);
	for (i = cutlen; i < rsHandle -> nn - rsHandle -> nroots; i++)
	   d [i - cutlen] = rf [i];
	return ret;
}

void	rsDecoder::encode (const DTYPE *data, DTYPE *bb){
int i, j;
DTYPE feedback;

	memset (bb, 0, rsHandle -> nroots * sizeof (DTYPE));

	for (i = 0; i < rsHandle -> nn - rsHandle -> nroots; i++){
	   feedback = rsHandle -> index_of [data [i] ^ bb [0]];
	   if (feedback != rsHandle -> nn){ /* feedback term is non-zero */
	      for (j = 1; j < rsHandle -> nroots; j++)
	         bb [j] ^= rsHandle -> alpha_to [modnn (rsHandle,
	              feedback + rsHandle -> genpoly [rsHandle -> nroots - j])];
	   }
/*	Shift */
	   memmove (&bb [0], &bb[1], sizeof (DTYPE) * (rsHandle -> nroots - 1));
	   if (feedback != rsHandle -> nn)
	      bb [rsHandle -> nroots - 1] =
	          rsHandle -> alpha_to [modnn (rsHandle,
	                                    feedback + rsHandle -> genpoly [0])];
	   else
	      bb [rsHandle -> nroots - 1] = 0;
	}
}

void	rsDecoder::enc (const DTYPE *r, DTYPE *d, int16_t cutlen) {
DTYPE rf [rsHandle -> nn];
int16_t i;
int16_t	ret;
DTYPE bb [rsHandle -> nroots];

	memset (rf, 0, cutlen * sizeof (DTYPE));
	for (i = cutlen; i < rsHandle -> nn; i++)
	   rf [i] = r[i - cutlen];

	encode (rf, bb);
	for (i = cutlen; i < rsHandle -> nn - rsHandle -> nroots; i++)
	   d [i - cutlen] = rf [i];
	for (i = 0; i < rsHandle -> nroots; i ++)
	   d [rsHandle -> nn - cutlen - rsHandle -> nroots + i] = bb [i];
}

