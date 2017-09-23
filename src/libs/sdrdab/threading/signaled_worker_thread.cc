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
#include <pthread.h>
#include <cstdio>
#include "scoped_lock.h"
#include "signaled_worker_thread.h"

void* SignaledWorkerThread::helper(void* context) {
    ((SignaledWorkerThread*)context)->loop();
    return NULL;
}

SignaledWorkerThread::SignaledWorkerThread() :
    initialized_(false),
    finished_flag_(false),
    is_suspended_flag_(true),
    id_(-1)
{}

SignaledWorkerThread::~SignaledWorkerThread() {
    if (initialized_) {
        cancel_thread();
    }
}

bool SignaledWorkerThread::is_suspended() {
    ScopedLock lock(lock_);
    return is_suspended_flag_;
}

int SignaledWorkerThread::init(void (*worker)(void*), void* worker_arg, int id, BlockingQueue<int>* signal_queue) {
    worker_ = worker;
    worker_arg_ = worker_arg;
    is_suspended_flag_ = true;
    id_ = id;
    signal_queue_ = signal_queue;
    initialized_ = true;

    int ret;
    if ((ret = pthread_mutex_init(&lock_, NULL)) != 0)
        return ret;
    if ((ret = pthread_cond_init(&cond_, NULL)) != 0)
        return ret;
    return pthread_create(&thread_, NULL, &SignaledWorkerThread::helper, this);
}


void SignaledWorkerThread::loop() {
#ifdef _GNU_SOURCE
    // if possible, set the name of thread. use
    // `ps H -C a.out -o 'pid tid cmd comm'` to see it
    char name[16];
    snprintf(name, sizeof(name), "sdrdab-id-%d", id_);
    pthread_setname_np(pthread_self(), name);
#endif
    //fprintf(stderr, "Threading: Thread #%d initialized.\n", id_);
    //fprintf(stderr, "Threading: thread #%d suspended.\n", id_);
    wait_for_resume();
    while (!finished_flag_) {
        //fprintf(stderr, "Threading: Thread #%d resumed.\n", id_);
        worker_(worker_arg_);
        suspend_thread();
        signal_queue_->push(id_);
        //fprintf(stderr, "Threading: thread #%d suspended.\n", id_);
        wait_for_resume();
    }
    //fprintf(stderr, "Threading: thread #%d finished.\n", id_);
}

void SignaledWorkerThread::wait_for_resume() {
    ScopedLock lock(lock_);
    while (is_suspended_flag_ && !finished_flag_)
        pthread_cond_wait(&cond_, &lock_);
}

void SignaledWorkerThread::resume_thread() {
    ScopedLock lock(lock_);
    if (!is_suspended_flag_) {
        fprintf(stderr, "\n!!!!SignaledWorkerThread: trying to resume not suspended thread #%d!!!!\n", id_);
    }
    is_suspended_flag_ = false;
    pthread_cond_signal(&cond_);
}

void SignaledWorkerThread::suspend_thread() {
    ScopedLock lock(lock_);
    is_suspended_flag_ = true;
}

void SignaledWorkerThread::cancel_thread() {
    pthread_cancel(thread_);
    pthread_mutex_destroy(&lock_);
    pthread_cond_destroy(&cond_);
    initialized_ = false;
}

void SignaledWorkerThread::finish() {
    {
        // acquire lock to change flags
        ScopedLock lock(lock_);
        is_suspended_flag_ = false;
        finished_flag_ = true;
        // send signal that flags have changed to resume thread
        pthread_cond_signal(&cond_);
        // end of scope, lock_ will be released
    }

    // thread has been resumed, let's wait until it exits from the while loop
    pthread_join(thread_, NULL);
    initialized_ = false;
}
