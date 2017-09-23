/**
 * @class ReedSolomon
 * @brief Decoding with Reed-Solomon algorithm
 *
 * @author Michal Babiuch babiuch.michal@gmail.com (ReedSolomon::ReedSolomonCorrection)
 * @author Jaroslaw Bulat kwant@agh.edu.pl refactoring, cleanUP
 *
 * @date 7 July 2016 - version 2.0 beta
 * @date 1 November 2016 - version 2.0
 * @version 2.0
 * @copyright Copyright (c) 2016 Michał Jurczak
 *
 * @par License
 *
 * @todo:rewrite code from scratch, remove rscode
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


#ifndef REEDSOLOMON_H_
#define REEDSOLOMON_H_

#include <iostream>
#include <stdint.h>

extern "C" {
#include "../../rscode/ecc.h"
}

class ReedSolomon {
public:

	/**
	 * call rscode
	 */
	size_t ReedSolomonCorrection(uint8_t *data, int32_t size);
};

#endif /* ReedSolomon_H_ */
