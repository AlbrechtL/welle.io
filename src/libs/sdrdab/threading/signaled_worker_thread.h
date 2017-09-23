/**
 * @class SignaledWorkerThread
 * @brief Wrapper for threads that allow running small batches of work without constantly creating/destroying threads. It also supports notifying when the work is done.
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
#ifndef SIGNALED_WORKER_THREAD_H
#define SIGNALED_WORKER_THREAD_H

#include <pthread.h>
#include "blocking_queue.h"

class SignaledWorkerThread {
    private:
        // thread
        pthread_t thread_;
        // cond used to signal thread resume
        pthread_cond_t cond_;
        // lock protecting is_suspended_flag_ from unsafe access
        pthread_mutex_t lock_;
        // flag saying if the thread is waiting for resume signal
        bool is_suspended_flag_;
        // flag saying that thread should finish
        bool finished_flag_;
        // id_ that will be inserted into signal_queue_ when worker_ finishes
        int id_;
        // whether init() has been called
        bool initialized_;
        // worker_ that contains real code that does something
        void (*worker_)(void*);
        // argument passed to worker_
        void* worker_arg_;
        // queue for signaling that worker_ has finished doing something
        BlockingQueue<int>* signal_queue_;

        // helper that allows us to call member function loop() using pthread_create
        static void* helper(void* context);

        // main loop that calls worker_ every time it gets resume signal
        void loop();

        // wait_for_resume() blocks until resume_thread() is called
        void wait_for_resume();

        // sets is_suspended_flag_ to 1
        void suspend_thread();

    public:
        SignaledWorkerThread();
        ~SignaledWorkerThread();

        /**
         * Set parameters, create POSIX thread and wait for resume
         * @param worker function that will be run in threadon each resume
         * @param worker_arg parameter passed to worker function
         * @param id thread id, will be pushed to signal_queue when worker returns
         * @param signal_queue queue to push notifications to
         * @return 0 on success
         */
        int init(void (*worker)(void*), void* worker_arg, int id, BlockingQueue<int>* signal_queue);

        /**
         * Run worker once in thread.
         * Thread id will be pushed to event_queue when worker return..
         * Thread will suspend again when worker returns.
         */
        void resume_thread();

        /**
         * Nice way to finish thread (it will wait until worker returns if it's running)
         */
        void finish();

        /**
         * Ugly way to finish thread (think kill -9)
         */
        void cancel_thread();

        // check if thread is currently suspended
        bool is_suspended();
};

#endif
