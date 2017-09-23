/*
 * @class AbstractSrc
 *
 * @author: Kacper Patro patro.kacper@gmail.com
 * @date May 24, 2015
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

#ifndef SRC_ABSTRACT_SRC_H_
#define SRC_ABSTRACT_SRC_H_

/**
 * @class AbstractSrc
 * @brief Abstract class for sources
 *
 * @author Kacper Patro patro.kacper@gmail.com
 * @copyright Public domain
 * @pre
 */
class AbstractSrc {
    public:
        AbstractSrc();
        virtual ~AbstractSrc();

        /**
         * Sets source properties, before linking
         * @param[in,out] other_data Other data pointer, may be pointer to structure being linked
         */
        virtual void SetSrc(void *other_data) = 0;

        /**
         * Links source element
         */
        virtual void LinkSrc() = 0;

        /**
         * Returns element name
         * @return Element name
         */
        virtual const char *name() const = 0;

        /**
         * Resets source state
         */
        virtual void ResetSrc() = 0;

};

#endif /* SRC_ABSTRACT_SRC_H_ */
