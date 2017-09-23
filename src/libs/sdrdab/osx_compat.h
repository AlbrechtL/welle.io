/**
 * @file osx_compat.cc
 *
 *
 * @author Jakub Bernardy kbernady@gmail.com (clock_gettime() 90%,pthread_mutex_timedlock() 90%)
 * @author Adrian Włosiak adwlosiakh@gmail.com (clock_gettime() 10%, pthread_mutex_timedlock() 10%)
 * @date 7 July 2015 - version 1.0 beta
 * @date 7 July 2016 - version 2.0 beta
 * @date 1 November 2016 - version 2.0
 * @version 2.0
 * @copyright Copyright (c) 2016 Jakub Bernardy, Adrian Włosiak
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

#include <mach/clock.h>
#include <mach/mach.h>
#include <pthread.h>

#define TIME_UTC 1
#define CLOCK_REALTIME 0

// OS X does not have clock_gettime, use clock_get_time
int clock_gettime(int clk_id, struct timespec *tp);

//definition of pthread_mutex_timedlock for OSX:
#if !(defined(_POSIX_TIMEOUTS) && (_POSIX_TIMEOUTS >= 200112L) && defined(_POSIX_THREADS) && (_POSIX_THREADS >= 200112L))
int pthread_mutex_timedlock(pthread_mutex_t *mutex, const struct timespec *abstime);
#endif
