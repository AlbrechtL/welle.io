/* Include file to configure the RS codec for character symbols
 *
 * Copyright 2002, Phil Karn, KA9Q
 * May be used under the terms of the GNU General Public License (GPL)
 */

#ifndef __REED_SOLOMON
#define __REED_SOLOMON

#include <stdint.h>
#include <vector>
#include "galois.h"

class ReedSolomon
{
    public:
        ReedSolomon(uint16_t symsize   = 8,
                    uint16_t gfpoly    = 0435,
                    uint16_t fcr       = 0,
                    uint16_t prim      = 1,
                    uint16_t nroots    = 10);

        int16_t dec(const uint8_t *data_in, uint8_t *data_out, int16_t cutlen);
        void    enc(const uint8_t *data_in, uint8_t *data_out, int16_t cutlen);

    private:
        const uint16_t symsize;   /* Bits per symbol */
        const uint16_t codeLength;/* Symbols per block (= (1<<mm)-1) */
        const uint16_t nroots;    /* Number of generator roots = number of parity symbols */
        const uint8_t  fcr;       /* First consecutive root, index form */
        const uint8_t  prim;      /* Primitive element, index form */
        uint8_t  iprim;     /* prim-th root of 1, index form */
        galois   myGalois;
        std::vector<uint8_t> generator; /* Generator polynomial */

        bool computeSyndromes(
                const std::vector<uint8_t>& data,
                std::vector<uint8_t>& syndromes);

        uint8_t getSyndrome(const std::vector<uint8_t>& data, uint8_t root);
        uint16_t computeLambda(
                const std::vector<uint8_t>& syndromes,
                std::vector<uint8_t>& lambda);

        int16_t  computeErrors(const std::vector<uint8_t>& lambda,
                                uint16_t deg_lambda,
                                std::vector<uint8_t>& rootTable,
                                std::vector<uint8_t>& locTable);

        uint16_t computeOmega(
                const std::vector<uint8_t>& syndromes,
                const std::vector<uint8_t>& lambda,
                uint16_t deg_lambda,
                std::vector<uint8_t>& omega);

        std::vector<uint8_t> encode_rs(const std::vector<uint8_t>& data_in);
        int16_t  decode_rs(std::vector<uint8_t> &data);
};

#endif
