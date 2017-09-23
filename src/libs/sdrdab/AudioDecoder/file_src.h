/*
 * @class FileSrc
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

#ifndef SRC_FILE_SRC_H_
#define SRC_FILE_SRC_H_

#include "abstract_src.h"
#include "player.h"

/**
 * @class FileSrc
 * @brief Class used to read from audio files
 *
 * @author Kacper Patro patro.kacper@gmail.com
 * @copyright Public domain
 * @pre
 */
class FileSrc: public AbstractSrc {
    public:
        /**
         * Constructor of FileSrc
         * @param[in] path Path to input file
         */
        FileSrc(const char *path);
        virtual ~FileSrc();

        void SetSrc(void *player_data);
        void LinkSrc();
        const char *name() const;
        void ResetSrc();

    private:
        /**
         * @struct Data
         * @brief This struct contains specific for FileSrc class elements
         */
        struct Data {
            FileSrc *abstract_src;  /**< Pointer to "this" src element */
            GstElement *src;    /**< Src element for GStreamer */
            Player::Data *player_data;  /**< Pointer to core Player data */
        };

        Data *data_;    /**< Pointer to internal Data element */

        const char *path_;  /**< Path to input file */

};

#endif /* SRC_FILE_SRC_H_ */
