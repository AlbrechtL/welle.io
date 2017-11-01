/** @file galois_field_arithmethic.cc
 *  @brief Source code file for Galois Field arithmetic.
 *  
 *  This is a source code file defining functions that evaluate operations over GF(2^m).
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

#include <iostream>
#include <string.h>
#include <stdint.h>
#include <algorithm>
#include "galois_field_arithmetic.h"

static const int USED_UINT_SIZE = 8;
static const int MAX_NUM_OF_ELEMENTS = 1<<USED_UINT_SIZE;

namespace gf {

	size_t numOfElements_;
	size_t numOfNonZeroElements_;

	uint8_t v2p_[MAX_NUM_OF_ELEMENTS];
	uint8_t p2v_[MAX_NUM_OF_ELEMENTS];

	void initialise(int m, uint8_t primitivePoly) {

		if (m > USED_UINT_SIZE)
			std::cerr << "Error: Galois Field: too big m value!\n";
		else {
			numOfElements_ = 1<<m;
			numOfNonZeroElements_ = numOfElements_ - 1;

			memset(p2v_, 0, numOfElements_*sizeof(uint8_t));

			for (int i = 0; i < m; i++)
				p2v_[i] = 1<<i;

			p2v_[m] = primitivePoly;

			for (size_t i = m+1; i < numOfElements_-1; i++)
				for (size_t j = 0; j < m; j++)
					if ((1<<j) & primitivePoly)
						p2v_[i] ^= p2v_[j+i-m];

		    p2v_[numOfElements_-1] = 1;

			memset(v2p_, 0, numOfElements_*sizeof(uint8_t));

			for (size_t i = 1; i < numOfElements_-1; i++) {
				v2p_[p2v_[i]] = i;
			}
		}

	}

	uint8_t mul(uint8_t a, uint8_t b) {
		if (a == 0 || b == 0)
			return 0;
		else
			return p2v_[(v2p_[a] + v2p_[b]) % numOfNonZeroElements_];
	}

	uint8_t inv(uint8_t a) {
		return p2v_[numOfNonZeroElements_-v2p_[a]];
	}

	uint8_t div(uint8_t a, uint8_t b) {
		return mul(a, inv(b));
	}

	using std::min;
	using std::max;

	void conv(uint8_t a[], uint8_t b[], uint8_t ans[], size_t la, size_t lb) {
	    for (size_t i = 1; i < la+lb; ++i) {
	        ans[i-1] = 0;

	        size_t start = 1;
			if(lb<i)
				start = i-lb+1;

	        for (size_t j = start;  j < min(i, la) + 1; ++j)
	            ans[i-1] ^= mul(a[j-1], b[i-j]);
	    }
	}

}
