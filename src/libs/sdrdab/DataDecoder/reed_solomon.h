/** @file reed_solomon.h
 *  @brief Header file for Reed-Solomon decoder.
 *
 *  This is a header file declaring Reed-Solomon decoder class.
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

#ifndef REED_SOLOMON_H_
#define REED_SOLOMON_H_

#include <stdint.h>

/** @brief Reed-Solomon decoder
 *
 *	This is a Reed-Solomon decoder for normal and shorthened RS codes over GF(2^m).
 *	It is adapted to DAB+:
 *  - GF(2^8) was selected, with x^8 + x^4 + x^3 + x^2 + 1 polynomial,
 *	- RS generator polynimial roots start with alpha^0 (b = 0);
 *	- code parameters are (120, 110) t=5, which is a shorthened (255, 245) t=5 RS code,
 *  - default constructor has default parameters,
 *  - syndrome calculation and block decoding were adjusted.
 */
class ReedSolomon {
private:
	static const int MAX_T = 5; /**< Max correction capability */
	static const int MAX_RECEIVED = 120; /**< Max received word length */

	size_t correctionCapability_; /**< Error correction capability */
	size_t messageLength_; /**< Information word length */
	size_t receivedLength_; /**< Code word length */
	size_t shorteningSize_;

	size_t numberOfWords_; /**< Number of code words in a block */

	uint8_t alfaPowers_[2*MAX_T][MAX_RECEIVED];	/**< Powers of primitive GF element. Look-up table. */
	uint8_t syndrome_[2*MAX_T];
	uint8_t errorLocatorPoly_[MAX_T+1];
	size_t errorLocatorPolyOrder_;
	uint8_t errorLocatorPolyRoots_[MAX_T+1];
	uint8_t errorPositions_[MAX_T+1];
	uint8_t errorValues_[MAX_T+1];

public:
	/** @brief Default constructor
	 *
	 *	Creates decoder instance. DAB+ default parameters.
	 *
	 *	@param[in] n 				Information word length
	 *	@param[in] k 				Code word length
	 *	@param[in] shorteningSize 	Number of zeros in info and code words
	 */
	ReedSolomon(int n = 255, int k = 245, int shorteningSize = 135);
	/** @brief Default destructor */
	~ReedSolomon() {};


	// HIGH-LEVEL INTERFACE
	/** @brief Correct a frame secured by RS code
	 *
	 *	High-level function. Finds errors in RS code secured frame and corrects them.
	 *  Adjusted to DAB+.
	 *
	 *	@par Input description
	 *	"receivedBlock" is an array of codewords. Codeword has "receivedLength" number of coeficients.
	 *	By default 120. Those are sorted in such a way that most significant coefficients are first.
	 *	For example, if blockSize = 14, and receivedLength = 120, there are 1680 numbers in receivedBlock.
	 *	- [0] 		- most significant coefficient of first codeword
	 *	- [1]		- most significant coefficient of second codeword
	 *	- ...
	 *	- [1678]	- least significant coefficient of 13th codeword
	 *	- [1679]	- least significant coefficient of 14th codeword
	 *
	 *	@par Operation
	 *	correctWord function is called on every codeword. Number of decoder errors is returned.
	 *	Decoder error occurs when there are more than t errors in a codeword.
	 *
	 *	@param[in,out] receivedBlock	Block of data (code words)
	 *	@param[in] blockSize			Number of codewords in a block
	 *	@return							Number of decoder errors in a block
	 */
	int ReedSolomonCorrection(uint8_t *receivedBlock, int blockSize);


	// LOW-LEVEL INTERFACE
	/** @brief Syndrome calculation
	 *
	 *	Low-level function. Calculates syndrome of receivedWord to syndrome array.
	 *  Adjusted to DAB+.
	 *
	 *	@param[in] receivedWord		Received code word
	 */
	void calculateSyndrome(uint8_t *receivedWord);

	/** @brief Syndrome equals zero predicate
	*
	*	Low-level function. Checks if syndrome of receivedWord equals zero.
	*
	*	@return		true if syndrome equals zero, false otherwise
	*/
	bool syndromeEqualsZero();

	/** @brief Berlekamp-Massey algorithm
	 *
	 *	Low-level function. Finds shortest Error Locator Polynomial.
	 */
	void berlekampMassey();

	/** @brief Chien search algorithm
	 *
	 *	Low-level function. Finds roots of Error Locator Polynomial.
	 *
	 *	@return		true if roots found, false in case of error
	 */
	bool chienSearch();

	/** @brief Forney algorithm
	 *
	 *	Low-level function. Finds error values.
	 */
	void forney();
	
	/** @brief Correct received code word
	 *
	 *	Low-level function. Corrects errors in received codeword.
	 *	Adjusted to DAB+.
	 *
	 *	@param[in] receivedWord		Received code word
	 *	@return 	true if corrected, false if decoder error
	 */
	bool correctWord(uint8_t *receivedWord);

};

#endif /* REED_SOLOMON_H_ */
