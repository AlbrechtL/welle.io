/*
 * @class AbstractSink
 *
 * @author: Kacper Patro patro.kacper@gmail.com
 * @date May 13, 2015
 *
 * @version 1.0 beta
 * @copyright Copyright (c) 2015 Kacper Patro
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
 *
 */

#ifndef SRC_ABSTRACT_SINK_H_
#define SRC_ABSTRACT_SINK_H_

/**
 * @class AbstractSink
 * @brief Abstract class for sinks
 *
 * @author Kacper Patro patro.kacper@gmail.com
 * @copyright Public domain
 * @pre
 */
class AbstractSink {
    public:
        AbstractSink();
        virtual ~AbstractSink();

        /**
         * Initializes sink structures
         * @param[in,out] other_data Other data pointer, may be pointer to structure being linked
         */
        virtual void InitSink(void *other_data) = 0;

        /**
         * Returns element name
         * @return Element name
         */
        virtual const char *name() const = 0;

        /**
         * This one should be called when unlinking sink
         */
        virtual void Finish() = 0;

        /**
         * Checks if sink is already linked
         * @return Current linkage status
         */
        virtual bool linked() const = 0;

        /**
         * Equality operator overload, compares sinks names
         * @return True when elements equal, false otherwise
         */
        bool operator ==(const AbstractSink &) const;

};

#endif /* SRC_ABSTRACT_SINK_H_ */
