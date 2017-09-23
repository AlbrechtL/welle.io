/**
 * @class ScopedLock
 * @brief Implementation of RAII (https://en.wikipedia.org/wiki/Resource_Acquisition_Is_Initialization) for mutex locking.
 *
 * @author Kacper Żuk <sdr@kacperzuk.pl>
 * @date 7 July 2016 - version 2.0 beta
 * @date 1 November 2016 - version 2.0
 * @version 2.0
 * @copyright Copyright (c) 2016 Kacper Żuk
 *
 * @par License
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

#ifndef SCOPED_LOCK_H
#define SCOPED_LOCK_H

#include <pthread.h>

class ScopedLock {
    private:
        pthread_mutex_t *lock_;

    public:
        ScopedLock(pthread_mutex_t & mutex);
        ~ScopedLock();
};

#endif
