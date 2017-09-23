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

#ifdef __MACH__
#include "osx_compat.h"
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <errno.h> //[EBUSY]?

extern int clock_gettime(int clk_id, struct timespec *tp) {
    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);

    tp->tv_sec = mts.tv_sec;
    tp->tv_nsec = mts.tv_nsec;

    return true; //FIXME
}

extern int pthread_mutex_timedlock(pthread_mutex_t *mutex, const struct timespec *abstime) {
    int rc;
    struct timespec cur, dur;

    /* Try to acquire the lock and, if we fail, sleep for 5ms. */
    while ((rc = pthread_mutex_trylock(mutex)) == EBUSY) {
        clock_gettime(TIME_UTC, &cur);

        if ((cur.tv_sec > abstime->tv_sec) || ((cur.tv_sec == abstime->tv_sec) && (cur.tv_nsec >= abstime->tv_nsec)))
            break;

        dur.tv_sec = abstime->tv_sec - cur.tv_sec;
        dur.tv_nsec = abstime->tv_nsec - cur.tv_nsec;
        if (dur.tv_nsec < 0) {
            dur.tv_sec--;
            dur.tv_nsec += 1000000000;
        }

        if ((dur.tv_sec != 0) || (dur.tv_nsec > 5000000)) {
            dur.tv_sec = 0;
            dur.tv_nsec = 5000000;
        }

        nanosleep(&dur, NULL);
    }

    return rc;
}
#endif
