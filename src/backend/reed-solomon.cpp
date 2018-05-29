/* Initialize a RS codec
 *
 * Copyright 2002 Phil Karn, KA9Q
 * May be used under the terms of the GNU General Public License (GPL)
 */
#include "reed-solomon.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>

using namespace std;

/*
 *  Reed-Solomon decoder
 *  Copyright 2002 Phil Karn, KA9Q
 *  May be used under the terms of the GNU General Public License (GPL)
 */
/*
 *  Rewritten - and slightly adapted while doing so -
 *  as a C++ class for use in the sdr-j dab decoder(s)
 *  Copyright 2015 Jan van Katwijk
 *  May be used under the terms of the GNU General Public License (GPL)
 */

#define min(a,b)    ((a) < (b) ? (a) : (b))

/* Initialize a Reed-Solomon codec
 * symsize = symbol size, bits (1-8)
 * gfpoly = Field generator polynomial coefficients
 * fcr = first root of RS code generator polynomial, index form, 0
 * prim = primitive element to generate polynomial roots
 * nroots = RS code generator polynomial degree (number of roots)
 */

ReedSolomon::ReedSolomon(
        uint16_t symsize,
        uint16_t gfpoly,
        uint16_t fcr,
        uint16_t prim,
        uint16_t nroots) :
    symsize(symsize),
    codeLength((1 << symsize) - 1),
    nroots(nroots),
    fcr(fcr),
    prim(prim),
    myGalois(symsize, gfpoly),
    generator(nroots + 1)
{
        int i, j, root, iprim;

        for (iprim = 1; (iprim % prim) != 0; iprim += codeLength);
        this->iprim    = iprim / prim;
        generator[0] = 1;

        for (i = 0, root = fcr * prim; i < nroots; i++, root += 1) {
            generator[i + 1] = 1;
            for (j = i; j > 0; j--){
                if (generator[j] != 0) {
                    uint16_t p1 = myGalois.multiply_power (
                            myGalois.poly2power (generator[j]),
                            root);
                    generator[j] = myGalois.add_poly (
                            generator[j - 1],
                            myGalois.power2poly (p1));

                }
                else {
                    generator[j] = generator[j - 1];
                }
            }

            /*  rsHandle->genpoly[0] can never be zero */
            generator[0] =
                myGalois.power2poly (
                        myGalois.multiply_power (root,
                            myGalois.poly2power (generator[0])));
        }
        for (i = 0; i <= nroots; i ++)
            generator[i] = myGalois.poly2power (generator[i]);
    }

//  Basic encoder, returns the parity bytes
std::vector<uint8_t> ReedSolomon::encode_rs(const std::vector<uint8_t>& data_in)
{
    int i, j;
    uint8_t feedback;

    vector<uint8_t> bb(nroots);

    for (i = 0; i < codeLength - nroots; i++){
        feedback = myGalois.poly2power (
                myGalois.add_poly(data_in[i], bb[0]));
        if (feedback != codeLength) { /* feedback term is non-zero */
            for (j = 1; j < nroots; j++) {
                bb[j] = myGalois.add_poly (bb[j],
                        myGalois.power2poly (
                            myGalois.multiply_power (feedback,
                                generator[nroots - j])));
            }
        }
        /*  Shift */
        memmove (&bb[0], &bb[1], sizeof (bb[0]) * (nroots - 1));
        if (feedback != codeLength)
            bb[nroots - 1] =
                myGalois.power2poly (
                        myGalois.multiply_power (feedback,
                            generator[0]));
        else
            bb[nroots - 1] = 0;
    }

    return bb;
}

void ReedSolomon::enc(const uint8_t *r, uint8_t *d, int16_t cutlen)
{
    vector<uint8_t> rf(codeLength);

    for (int i = cutlen; i < codeLength; i++) {
        rf[i] = r[i - cutlen];
    }

    const auto bb = encode_rs(rf);
    for (int i = cutlen; i < codeLength - nroots; i++) {
        d[i - cutlen] = rf[i];
    }

    //  and the parity bytes
    for (int i = 0; i < nroots; i ++) {
        d[codeLength - cutlen - nroots + i] = bb[i];
    }
}

int16_t ReedSolomon::dec(const uint8_t *r, uint8_t *d, int16_t cutlen)
{
    vector<uint8_t> rf(codeLength);

    for (int i = cutlen; i < codeLength; i++) {
        rf[i] = r[i - cutlen];
    }

    int16_t ret = decode_rs(rf);

    for (int i = cutlen; i < codeLength - nroots; i++) {
        d[i - cutlen] = rf[i];
    }

    return ret;
}

int16_t ReedSolomon::decode_rs(std::vector<uint8_t> &data)
{
    vector<uint8_t> syndromes(nroots);
    //  returning syndromes in poly
    if (computeSyndromes(data, syndromes))
        return 0;

    vector<uint8_t> lambda(nroots + 1);
    //  Step 2: Berlekamp-Massey
    //  lambda in power notation
    uint16_t lambda_degree = computeLambda(syndromes, lambda);

    vector<uint8_t> rootTable(nroots);
    vector<uint8_t> locTable(nroots);
    //  Step 3: evaluate lambda and compute the error locations (chien)
    int16_t rootCount = computeErrors(lambda, lambda_degree, rootTable, locTable);

    if (rootCount < 0)
        return -1;

    vector<uint8_t> omega(nroots + 1);
    uint16_t omega_degree = computeOmega(syndromes, lambda, lambda_degree, omega);
    /*
     *  Compute error values in poly-form.
     *  num1 = omega (inv (X (l))),
     *  num2 = inv (X (l))**(FCR-1) and
     *  den = lambda_pr(inv(X(l))) all in poly-form
     */
    uint16_t num1, num2, den;
    for (int j = rootCount - 1; j >= 0; j--) {
        num1 = 0;
        for (int i = omega_degree; i >= 0; i--) {
            if (omega[i] != codeLength) {
                uint16_t tmp = myGalois.multiply_power(omega[i],
                        myGalois.pow_power(i, rootTable[j]));
                num1 = myGalois.add_poly(num1,
                        myGalois.power2poly(tmp));
            }
        }
        uint16_t tmp = myGalois.multiply_power(
                myGalois.pow_power(
                    rootTable[j],
                    myGalois.divide_power(fcr, 1)),
                codeLength);
        num2 = myGalois.power2poly(tmp);
        den = 0;
        /*
         *  lambda[i + 1] for i even is the formal derivative
         *  lambda_pr of lambda[i]
         */
        for (int i = min (lambda_degree, nroots - 1) & ~1;
                i >= 0; i -=2 ) {
            if (lambda[i + 1] != codeLength) {
                uint16_t tmp = myGalois.multiply_power (lambda[i + 1],
                        myGalois.pow_power (i, rootTable[j]));
                den = myGalois.add_poly (den, myGalois.power2poly (tmp));
            }
        }

        if (den == 0) {
            //        fprintf (stderr, "den = 0, (count was %d)\n", den);
            return -1;
        }
        /*  Apply error to data */
        if (num1 != 0) {
            if (locTable[j] >=  uint8_t (codeLength - nroots))
                rootCount --;
            else {
                uint16_t tmp1   = codeLength - myGalois.poly2power (den);
                uint16_t tmp2   = myGalois.multiply_power (
                        myGalois.poly2power (num1),
                        myGalois.poly2power (num2));
                tmp2        = myGalois.multiply_power (tmp2, tmp1);
                uint16_t corr   = myGalois.power2poly (tmp2);
                data[locTable[j]] =
                    myGalois.add_poly (data[locTable[j]], corr);
            }
        }

    }
    return rootCount;
}

//  Apply Horner on the input for root "root"
uint8_t ReedSolomon::getSyndrome(const std::vector<uint8_t>& data, uint8_t root)
{
    uint8_t syn = data[0];

    for (int j = 1; j < codeLength; j++){
        if (syn == 0)
            syn = data[j];
        else {
            uint16_t uu1 = myGalois.pow_power (
                    myGalois.multiply_power (fcr, root),
                    prim);
            syn = myGalois.add_poly (data[j],
                    myGalois.power2poly (
                        myGalois.multiply_power (
                            myGalois.poly2power (syn), uu1)));
            //                                                     (fcr + root) * prim)));
        }
    }
    return syn;
}

//  use Horner to compute the syndromes
bool ReedSolomon::computeSyndromes(
        const std::vector<uint8_t>& data, std::vector<uint8_t>& syndromes)
{
    uint16_t syn_error = 0;

    /* form the syndromes; i.e., evaluate data (x) at roots of g(x) */

    for (int i = 0; i < nroots; i++) {
        syndromes[i] = getSyndrome(data, i);
        syn_error |= syndromes[i];
    }

    return syn_error == 0;
}

//  compute Lambda with Berlekamp-Massey
//  syndromes in poly-form in, Lambda in power form out
uint16_t ReedSolomon::computeLambda(
        const std::vector<uint8_t>& syndromes,
        std::vector<uint8_t>& lambda)
{
    uint16_t K = 1, L = 0;
    vector<uint8_t> corrector(nroots + 1);
    int16_t deg_lambda = 0;

    //  Initializers:
    lambda[0] = 1;
    corrector[1] = 1;

    vector<uint8_t> oldLambda(nroots + 1);
    if (lambda.size() != oldLambda.size()) {
        throw logic_error("Invalid lambda vectors");
    }

    uint8_t error = syndromes[0];

    while (K <= nroots) {
        copy(lambda.begin(), lambda.end(), oldLambda.begin());

        //  Compute new lambda
        for (int i = 0; i < nroots + 1; i ++) {
            lambda[i] = myGalois.add_poly (lambda[i],
                    myGalois.multiply_poly (error,
                        corrector[i]));
        }

        if ((2 * L < K) && (error != 0)) {
            L = K - L;
            for (int i = 0; i < nroots + 1; i ++) {
                corrector[i] = myGalois.divide_poly (oldLambda[i], error);
            }
        }

        //  multiply x * C (x), i.e. shift to the right, the 0-th order term is left
        for (int i = nroots; i >= 1; i --) {
            corrector[i] = corrector[i - 1];
        }
        corrector[0] = 0;

        if (K < nroots) {
            //  and compute a new error except in the last iteration
            error = syndromes[K];
            for (int i = 1; i <= K; i ++)  {
                error = myGalois.add_poly(error,
                        myGalois.multiply_poly(syndromes[K - i],
                            lambda[i]));
            }
        }
        K += 1;
    } // end of Berlekamp loop

    for (int i = 0; i < nroots + 1; i ++) {
        if (lambda[i] != 0)
            deg_lambda = i;
        lambda[i] = myGalois.poly2power(lambda[i]);
    }
    return deg_lambda;
}

//  Compute the roots of lambda by evaluating the
//  lambda polynome for all (inverted) powers of the symbols
//  of the data (Chien search)
int16_t ReedSolomon::computeErrors(
        const std::vector<uint8_t>& lambda,
        uint16_t deg_lambda,
        std::vector<uint8_t>& rootTable,
        std::vector<uint8_t>& locTable)
{
    int16_t rootCount = 0;

    vector<uint8_t> workRegister = lambda;

    //  reg is lambda in power notation
    for (int i = 1, k = iprim - 1;
            i <= codeLength; i ++, k = (k + iprim)) {
        uint16_t result = 1;    // lambda[0] is always 1
        //  Note that for i + 1, the powers in the workregister just need
        //  to be increased by "j".
        for (int j = deg_lambda; j > 0; j --) {
            if (workRegister[j] != codeLength)  {
                workRegister[j] = myGalois.multiply_power (workRegister[j],
                        j);
                result = myGalois.add_poly (result,
                        myGalois.power2poly
                        (workRegister[j]));
            }
        }

        if (result != 0)        // no root
            continue;
        rootTable[rootCount] = i;
        locTable[rootCount] = k;
        rootCount++;
    }

    if (rootCount != deg_lambda)
        return -1;
    return rootCount;
}

/*
 *  Compute error evaluator poly
 *  omega(x) = s(x)*lambda(x) (modulo x**NROOTS)
 *  in power form, and  find degree (omega).
 *
 *  Note that syndromes are in poly form, while lambda in power form
 */
uint16_t ReedSolomon::computeOmega(
        const std::vector<uint8_t>& syndromes,
        const std::vector<uint8_t>& lambda,
        uint16_t deg_lambda,
        std::vector<uint8_t>& omega)
{
    int16_t deg_omega = 0;

    for (int i = 0; i < nroots; i++) {
        uint16_t tmp = 0;
        int j = (deg_lambda < i) ? deg_lambda : i;
        for (; j >= 0; j--) {
            if ((myGalois.poly2power (syndromes[i - j]) != codeLength) &&
                    (lambda[j] != codeLength)) {
                uint16_t res = myGalois.power2poly (
                        myGalois.multiply_power (
                            myGalois.poly2power (
                                syndromes[i - j]),
                            lambda[j]));
                tmp = myGalois.add_poly (tmp, res);
            }
        }

        if (tmp != 0)
            deg_omega = i;
        omega[i] = myGalois.poly2power(tmp);
    }

    omega[nroots] = codeLength;
    return deg_omega;
}

