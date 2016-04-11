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

/*	Multiply rsHandle -> genpoly[] by  @**(root + x) */
	   for (j = i; j > 0; j--){
	      if (rsHandle -> genpoly [j] != 0)
	         rsHandle -> genpoly [j] = rsHandle -> genpoly [j - 1] ^
	            rsHandle -> alpha_to [modnn (rsHandle,
	                                         rsHandle -> index_of[rsHandle ->genpoly[j]] + root)];
	      else
	         rsHandle -> genpoly[j] = rsHandle -> genpoly [j - 1];
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
int deg_lambda, el, deg_omega;
int i, j, r,k;
DTYPE u,q,tmp,num1,num2,den,discr_r;
DTYPE lambda [rsHandle -> nroots + 1],
      s [rsHandle -> nroots];	/* Err Locator poly and syndrome poly */
DTYPE b [rsHandle -> nroots +1],
	t [rsHandle -> nroots +1],
	omega [rsHandle -> nroots + 1];

DTYPE root [rsHandle -> nroots],
	reg [rsHandle -> nroots + 1],
	 loc [rsHandle -> nroots];
int syn_error, count;

/* form the syndromes; i.e., evaluate data (x) at roots of g(x) */
	for (i = 0; i < rsHandle -> nroots; i++)
	   s [i] = data [0];

	for (j = 1; j < rsHandle -> nn; j++){
	   for (i = 0; i < rsHandle -> nroots; i++){
	      if (s[i] == 0){
	         s[i] = data [j];
	      } else {
	         s[i] = data [j] ^
	                     rsHandle -> alpha_to [modnn (rsHandle,
	                                                  rsHandle -> index_of [s[i]] + (rsHandle -> fcr + i) * rsHandle -> prim)];
	      }
	   }
 	}

/*	Convert syndromes to index form, checking for nonzero condition */
	syn_error = 0;
	for (i = 0; i < rsHandle -> nroots; i++){
	   syn_error |= s [i];
	   s [i] = rsHandle -> index_of [s [i]];
	}

	if (syn_error == 0) {
/*	if syndrome is zero, data [] is a codeword and there are no
 *	errors to correct. So return data [] unmodified
 */
 	   count = 0;
	   goto finish;
	}

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
	   discr_r = 0;
	   for (i = 0; i < r; i++) {
	      if ((lambda [i] != 0) &&
	          (s [r - i - 1] != rsHandle -> nn)) {
	         discr_r ^= rsHandle -> alpha_to [modnn (rsHandle,
	                                                 rsHandle -> index_of [lambda [i]] + s [r-i-1])];
	      }
	   }

	   discr_r = rsHandle -> index_of [discr_r];	/* Index form */
	   if (discr_r == rsHandle -> nn) {
/*	2 lines below: B(x) <-- x*B(x) */
	      memmove (&b [1], b, rsHandle -> nroots * sizeof (b[0]));
	      b[0] = rsHandle -> nn;
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

 /*	Convert lambda to index form and compute deg (lambda(x)) */
	deg_lambda = 0;
	for (i = 0; i < rsHandle -> nroots + 1; i++){
	   lambda [i] = rsHandle -> index_of [lambda [i]];
	   if (lambda [i] != rsHandle -> nn)
	      deg_lambda = i;
	}

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
/*	If we've already found max possible roots,
 *	abort the search to save time
 */
	   ++count;
//	   if (++count == deg_lambda)
//	      break;
	}

	if (deg_lambda != count) {
/*
 *	deg (lambda) unequal to number of roots uncorrectable error detected
 */
	   count = -1;
	   goto finish;
  	}
/*
 *	Compute error evaluator poly omega(x) = s(x)*lambda(x)
 *	(modulo	x**NROOTS). in index form. Also find deg (omega).
 */
	deg_omega = 0;
	for (i = 0; i < rsHandle -> nroots; i++){
	   tmp = 0;
	   j = (deg_lambda < i) ? deg_lambda : i;
	   for (; j >= 0; j--){
	      if ((s[i - j] != rsHandle -> nn) &&
	          (lambda[j] != rsHandle -> nn))
	         tmp ^= rsHandle -> alpha_to [modnn (rsHandle,
	                                             s [i - j] + lambda [j])];
	   }

	   if (tmp != 0)
	      deg_omega = i;
	   omega [i] = rsHandle -> index_of [tmp];
	}

	omega [rsHandle -> nroots] = rsHandle -> nn;
/*
 *	Compute error values in poly-form.
 *	num1 = omega (inv (X (l))),
 *	num2 = inv (X (l))**(FCR-1) and
 *	den = lambda_pr(inv(X(l))) all in poly-form
 */
	for (j = count - 1; j >= 0; j--) {
	   num1 = 0;
	   for (i = deg_omega; i >= 0; i--) {
	      if (omega [i] != rsHandle -> nn)
	         num1  ^= rsHandle -> alpha_to [modnn (rsHandle,
	                                               omega [i] + i * root[j])];
	   }

	   num2 = rsHandle -> alpha_to [modnn (rsHandle,
	                                       root [j] *
	                                       (rsHandle -> fcr - 1) + 
	                                       rsHandle -> nn)];
	   den = 0;
/*
 *	lambda [i + 1] for i even is the formal derivative
 *	lambda_pr of lambda[i]
 */
	   for (i = min (deg_lambda, rsHandle -> nroots - 1) & ~1;
	                 i >= 0; i -=2) {
	      if (lambda [i+1] != rsHandle -> nn)
	         den ^= rsHandle -> alpha_to [modnn (rsHandle,
	                                             lambda [i + 1] + i * root [j])];
	   }

	   if (den == 0) {
	   fprintf (stderr, "den = 0, (count was %d)\n", den);
	      count = -1;
	      goto finish;
	   }

/*	Apply error to data */
	   if (num1 != 0) {
	      if (loc [j] >= rsHandle -> nn - rsHandle -> mm)
	         count --;
	      data [loc [j]] ^=
	             rsHandle -> alpha_to [modnn (rsHandle,
	                                      rsHandle -> index_of [num1] +
	                                      rsHandle -> index_of [num2] + 
	                                      rsHandle -> nn -
	                                      rsHandle -> index_of [den])];
 	   }
 	}
 finish:
	return count;
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
