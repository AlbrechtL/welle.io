/** @file galois_field_arithmethic.h
 *  @brief Header file for Galois Field arithmetic.
 *  
 *  This is a header file declaring functions that evaluate operations over GF(2^m).
 *  It is used by Reed-Solomon decoder.
 *  
 *  @author Szymon Kurzepa szymon.kurzepa@gmail.com
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

#ifndef GALOIS_FIELD_ARITHMETIC_H_
#define GALOIS_FIELD_ARITHMETIC_H_

#include <stdint.h>

/** @brief Galois Field namespace */
namespace gf {

	extern size_t numOfElements_;
	extern size_t numOfNonZeroElements_;

	extern uint8_t v2p_[]; /**< Vector notation to power notation conversion look-up table. */
	extern uint8_t p2v_[]; /**< Power notation to vector notation conversion look-up table. */

	/** @brief Galois Field intialisation.
	 *
	 *	This function computes conversion look-up tables and set field variables.
	 *  Should be called before using other operations.
	 *  
	 *  @param[in] m 				Field characteristic. Positive int. GF(2^m)
	 *  @param[in] primitivePoly 	Field generator polynomial without highest power. As int.
	 */
	void initialise(int m, uint8_t primitivePoly);

	/** @brief Multiplication over GF(2^m)
	 *  
	 *  @param[in] a 	GF(2^m) element in vector notation.
	 *  @param[in] b 	GF(2^m) element in vector notation.
	 *  @return			Product of a and b in vector notation.
	 */
	uint8_t mul(uint8_t a, uint8_t b);

	/** @brief Multiplicative inverse element over GF(2^m)
	 *  
	 *  @param[in] a 	GF(2^m) element in vector notation.
	 *  @return			Inverse element of a in vector notation.
	 */
	uint8_t inv(uint8_t a);

	/** @brief Division over GF(2^m)
	 *  
	 *  @param[in] a 	GF(2^m) element in vector notation.
	 *  @param[in] b 	GF(2^m) element in vector notation.
	 *  @return			Quotient of a and b in vector notation.
	 */
	uint8_t div(uint8_t a, uint8_t b);

	/** @brief Convolution over GF(2^m)
	 *
	 *	This function computes convolution of polynomials over GF(2^m).
	 *  This is the same as multiplication of those polynomials over GF(2^m).
	 *  
	 *  @param[in] a 		First polynomial. Lowest power coefficient first.
	 *  @param[in] b 		Second polynomial. Lowest power coefficient first.
	 *  @param[out] ans 	Answer polynomial. Lowest power coefficient first.
	 *  @param[in] la 		a length
	 *  @param[in] lb 		b length
	 */
	void conv(uint8_t a[], uint8_t b[], uint8_t ans[], size_t la, size_t lb);

}

#endif /* GALOIS_FIELD_ARITHMETIC_H_ */
