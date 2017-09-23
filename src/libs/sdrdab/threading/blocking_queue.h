/**
 * @class BlockingQueue
 * @brief Implementation of a thread-safe queue. If it's empty, it'll block threads trying to pull from it
 *
 *
 * @author Kacper Żuk <sdr@kacperzuk.pl>
 * @date 7 July 2016 - version 2.0 beta
 * @date 1 November 2016 - version 2.0
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


#ifndef BLOCKING_QUEUE_H
#define BLOCKING_QUEUE_H

#include <pthread.h>
#include <queue>
#include <list>
#include <cassert>

#include "scoped_lock.h"


template <class T>
class BlockingQueue {
    public:
        BlockingQueue() {
            pthread_mutex_init(&lock_, NULL);
            pthread_cond_init(&cond_, NULL);
        }

        ~BlockingQueue() {
            pthread_cond_destroy(&cond_);
            pthread_mutex_destroy(&lock_);
        }

        /**
         * Push new element to the end of queue
         * @param element element to be added to queue
         */
        void push(T element) {
            ScopedLock slock(lock_);
            queue_.push(element);
            pthread_cond_signal(&cond_);
        }

        /**
         * Remove all elements from queue
         */
        void clear() {
            ScopedLock slock(lock_);
            std::queue<T, std::list<T> > empty;
            std::swap(queue_, empty);
        }

        /**
         * Pull first element from queue.
         * @return First element from queue
         */
        T pull() {
            ScopedLock slock(lock_);

            while (queue_.empty()) {
                pthread_cond_wait(&cond_, &lock_);
            }

            assert(!queue_.empty());
            T t = queue_.front();
            queue_.pop();

            return t;
        }


        /**
         * Get number of elements in queue
         * @return number of elements in queue
         */
        typename std::queue<T>::size_type size() {
            ScopedLock slock(lock_);
            return queue_.size();
        }

    private:
        std::queue<T, std::list<T> > queue_;
        pthread_cond_t cond_;
        pthread_mutex_t lock_;
};

#endif
