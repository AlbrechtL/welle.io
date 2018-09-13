/* Main header for reduced libfec.
 *
 * The FEC code in this folder is
 * Copyright 2003 Phil Karn, KA9Q
 * May be used under the terms of the GNU Lesser General Public License (LGPL)
 */

#pragma once

#include <stdlib.h>

#include "char.h"
#include "rs-common.h"

/* Initialize a Reed-Solomon codec
 * symsize = symbol size, bits
 * gfpoly = Field generator polynomial coefficients
 * fcr = first root of RS code generator polynomial, index form
 * prim = primitive element to generate polynomial roots
 * nroots = RS code generator polynomial degree (number of roots)
 * pad = padding bytes at front of shortened block
 */
void *init_rs_char(int symsize,int gfpoly,int fcr,int prim,int nroots,int pad);

int decode_rs_char(void *p, data_t *data, int *eras_pos, int no_eras);

void encode_rs_char(void *p,data_t *data, data_t *parity);

void free_rs_char(void *p);

