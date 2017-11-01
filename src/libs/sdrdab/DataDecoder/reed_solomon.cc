/** @file reed_solomon.cc
 *  @brief Source code file for Reed-Solomon decoder.
 *
 *  This is a source code file defining functions of Reed-Solomon decoder class.
 *	Interface and processing were adapted to DAB+ demands.
 *
 *  @author Szymon Kurzepa szymon.kurzepa@gmail.com
 *  @author Jaroslaw Bulat kwant@agh.edu.pl (cleanup, refactoring, fix errors)
 * 
 *  @date 20 February 2017
 *  @version 1.0
 *  @copyright Copyright (c) 2017 Szymon Kurzepa
 *
 *  @par License
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <iostream>
#include <string.h>
#include <stdint.h>
#include "galois_field_arithmetic.h"
#include "reed_solomon.h"

using namespace gf;

ReedSolomon::ReedSolomon(int n, int k, int shorteningSize) {

	// Galois Field init
	initialise(8 , 29); //0x1D

	this->correctionCapability_ = (n-k)/2;
	this->messageLength_ = k - shorteningSize;
	this->receivedLength_ = n - shorteningSize;
	this->shorteningSize_ = shorteningSize;

	if (correctionCapability_ > MAX_T || receivedLength_ > MAX_RECEIVED)
		std::cerr << "Error: ReedSolomon decoder: static arrays too small!\n";
	else {
		// precompute alfaPowers for speed
		// those will be used later in syndrome calculation
		for (int col = 0; col < receivedLength_; col++) {
            alfaPowers_[0][col] = 1;
            alfaPowers_[1][col] = col;
		}

		for (int row = 2; row < 2*correctionCapability_; row++)
			for (int col = 0; col < receivedLength_; col++)
				alfaPowers_[row][col] = p2v_[ alfaPowers_[1][col] * row % numOfNonZeroElements_ ];

		for (int col = 0; col < receivedLength_; col++)
			alfaPowers_[1][col] = p2v_[ alfaPowers_[1][col] ];
	}
}

void ReedSolomon::calculateSyndrome(uint8_t *receivedWord) {
	memset(syndrome_, 0, 2*correctionCapability_*sizeof(uint8_t));

	for (size_t pos = 0; pos < 2*correctionCapability_; pos++)
		for (size_t i = 0; i < receivedLength_; i++)
			syndrome_[pos] ^=  mul(receivedWord[numberOfWords_*(receivedLength_-1-i)], alfaPowers_[pos][i]);
}

bool ReedSolomon::syndromeEqualsZero() {
	for (size_t i = 0; i < 2*correctionCapability_; i++)
		if (syndrome_[i] != 0) return false;

	return true;
}

void ReedSolomon::berlekampMassey() {
	size_t regLength = 0;
	uint8_t currentConnPoly[MAX_T+1];
	uint8_t previousConnPoly[MAX_T+1];
	uint8_t temp[MAX_T+1];
	uint8_t shifted[MAX_T+1];
	size_t shiftAmount = 1;
	uint8_t discrep = 0;
	uint8_t discrepPrev = 1;
	uint8_t sum = 0;

    memset(currentConnPoly, 0, (MAX_T+1)*sizeof(uint8_t));
    currentConnPoly[0] = 1;
    memset(previousConnPoly, 0, (MAX_T+1)*sizeof(uint8_t));
    previousConnPoly[0] = 1;
    memset(temp, 0, (MAX_T+1)*sizeof(uint8_t));
    memset(shifted, 0, (MAX_T+1)*sizeof(uint8_t));

	for (size_t k = 1; k < 2*correctionCapability_ + 1; k++) {
		sum = 0;
		for (size_t i = 1; i < regLength + 1 ; i++)
			sum ^= mul(currentConnPoly[i], syndrome_[k-i-1]);
		discrep = syndrome_[k-1] ^ sum;

		if(discrep == 0)
			shiftAmount++;
		else {
			if (2*regLength >= k) {
				// left shift, shiftAmount positions
				memset(shifted, 0, (correctionCapability_+1)*sizeof(uint8_t));
				if (correctionCapability_+1-shiftAmount > 0)
                    memcpy(shifted+shiftAmount, previousConnPoly, (correctionCapability_+1-shiftAmount)*sizeof(uint8_t));

				// scale shifted poly, subtract from current
				uint8_t factor = div(discrep, discrepPrev);
				for (size_t i = 0; i < correctionCapability_+1; i++)
					currentConnPoly[i] ^= mul(factor, shifted[i]);

				shiftAmount++;
			}
			else {
				memcpy(temp, currentConnPoly, (correctionCapability_+1)*sizeof(uint8_t));

				// left shift, shiftAmount positions
				memset(shifted, 0, (correctionCapability_+1)*sizeof(uint8_t));
				if (correctionCapability_+1-shiftAmount > 0)
                    memcpy(shifted+shiftAmount, previousConnPoly, (correctionCapability_+1-shiftAmount)*sizeof(uint8_t));

				// scale shifted poly, subtract from current
				uint8_t factor = div(discrep, discrepPrev);
				for (size_t i = 0; i < correctionCapability_+1; i++)
					currentConnPoly[i] ^= mul(factor, shifted[i]);

				regLength = k - regLength;

				memcpy(previousConnPoly, temp, (correctionCapability_+1)*sizeof(uint8_t));
				discrepPrev = discrep;
				shiftAmount = 1;

			}
		}
	}

	memcpy(errorLocatorPoly_, currentConnPoly, (correctionCapability_+1)*sizeof(uint8_t));
	errorLocatorPolyOrder_ = regLength;
}

bool ReedSolomon::chienSearch() {
	memset(errorLocatorPolyRoots_, 0, errorLocatorPolyOrder_*sizeof(uint8_t));
	size_t rootsFound = 0;
	uint8_t currentState[MAX_T+1];

	// copy from 1 to v
	memcpy(currentState+1, errorLocatorPoly_+1, errorLocatorPolyOrder_*sizeof(uint8_t));

	uint8_t sum;
	for (size_t i = 0; i < 255; i++) {
		sum = 1;
		for (size_t pos = 1; pos < errorLocatorPolyOrder_ + 1; pos++ )
			sum ^= currentState[pos];
		if (sum == 0)
			errorLocatorPolyRoots_[rootsFound++] = p2v_[i];
		if (rootsFound > errorLocatorPolyOrder_)
			return false;

		for (size_t pos = 1; pos < errorLocatorPolyOrder_ + 1; pos++ )
			currentState[pos] = mul(currentState[pos], alfaPowers_[1][pos]);
	}

	if (rootsFound == errorLocatorPolyOrder_)
        return true;
	else
		return false;
}

void ReedSolomon::forney() {
    // multiply S by LAMBDA to obtain OMEGA
    uint8_t omega[3*MAX_T]; // correct to be 2*MAX_T, and below as well
    uint8_t formalDerivative[MAX_T];
    conv(syndrome_, errorLocatorPoly_, omega, 2*correctionCapability_, errorLocatorPolyOrder_+1); // consider only first 2*t elements!!!

    // calculate LAMBDA'
    memset(formalDerivative, 0, (MAX_T-1)*sizeof(uint8_t));
    for (int i = 0; i < errorLocatorPolyOrder_; i = i + 2)
        formalDerivative[i] = errorLocatorPoly_[i+1];

    int omegaOrder = 2*correctionCapability_-1;
    int formalDerivativeOrder = errorLocatorPolyOrder_-1;

    // evaluate errors
    for (size_t i = 0; i < errorLocatorPolyOrder_; i++) {
        uint8_t root = errorLocatorPolyRoots_[i];

        uint8_t omegaEval = omega[0];
        uint8_t rootEval = root;
        for (size_t pos = 1; pos < omegaOrder+1; pos++) {
            omegaEval ^= mul(omega[pos], rootEval);
            rootEval = mul(rootEval, root);
        }

        uint8_t formalDerEval = formalDerivative[0];
        rootEval = mul(root, root);
        for (size_t pos = 2; pos < formalDerivativeOrder+1; pos = pos + 2) {
            formalDerEval ^= mul(formalDerivative[pos], rootEval);
            rootEval = mul(rootEval, root);
            rootEval = mul(rootEval, root);
        }

        errorValues_[i] = mul(inv(root), div(omegaEval, formalDerEval));
    }
}

bool ReedSolomon::correctWord(uint8_t *receivedWord) {
	calculateSyndrome(receivedWord);

	if (syndromeEqualsZero()){
        // std::cout << "zero syndrome" << std::endl;
		return true;	// no errors
    } else {
        // std::cout << "NOT zero syndrome" << std::endl;
		berlekampMassey();

		if(errorLocatorPolyOrder_ > correctionCapability_){ // too many errors to be corrected
            // std::cerr << "##############" << errorLocatorPolyOrder_ << std::endl;
            return false;
        }

		if (chienSearch() == false){	// too many errors to be corrected
            // std::cerr << "????????????" << errorLocatorPolyOrder_ << std::endl;
			return false;
        }

        for (size_t pos = 0; pos < errorLocatorPolyOrder_; pos++ ){
            // std::cerr << pos << " " << errorLocatorPolyOrder_ << " " << correctionCapability_ << std::endl;

        	if( v2p_[inv(errorLocatorPolyRoots_[pos])]+1 > receivedLength_ ){
//        		std::cerr << "##->" << +receivedLength_ << std::endl;
//        		std::cerr << "##->" << +v2p_[inv(errorLocatorPolyRoots_[pos])] << std::endl;
//        		std::cerr << "##->" << +inv(errorLocatorPolyRoots_[pos]) << std::endl;
       		// std::cerr << "### buffer overflow!!!" << std::endl;
            // std::cerr << pos << " " << errorLocatorPolyOrder_ << " " << correctionCapability_ << std::endl;
				// TODO: find and correct
        		continue;
        	}
            // std::cerr << std::endl;

			errorPositions_[pos] = receivedLength_ - v2p_[inv(errorLocatorPolyRoots_[pos])] - 1;
        }        

		forney();

        for (size_t pos = 0; pos < errorLocatorPolyOrder_; pos++ ){
        	if( v2p_[inv(errorLocatorPolyRoots_[pos])]+1 > receivedLength_ ){
//				 std::cerr << "buffer overflow!!!" << std::endl;
//				// TODO: find and correct
        		continue;
        	}
			receivedWord[errorPositions_[pos] * numberOfWords_] ^= errorValues_[pos];
        }

		return true;	// errors corrected
	}
}

int ReedSolomon::ReedSolomonCorrection(uint8_t *receivedBlock, int blockSize) {
	numberOfWords_ = blockSize/receivedLength_;

	size_t nbrOfDecoderErrorsInBlock = numberOfWords_;
	for (size_t i = 0; i < numberOfWords_; i++){
        // 636
        // if( i>100)
            // std::cerr << "-------------- " << i << std::endl;
		nbrOfDecoderErrorsInBlock -= correctWord(receivedBlock+i);
	}

	return nbrOfDecoderErrorsInBlock;
}
