/**
 *    Copyright (C) 2017
 *    Albrecht Lohofener (albrechtloh@gmx.de)
 *
 *    This file is based on SDRDAB project
 *
 * @class CFICData
 * @brief Convenient UserFICData_t storage
 *
 * @author Krzysztof Szczęsny, kaszczesny@gmail.com
 * @date Created on: 13 June 2015
 * @version 1.0
 * @copyright Copyright (c) 2015 Krzysztof Szczęsny
 * @pre sdrdab
 * @par License
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see \<http://www.gnu.org/licenses/\>.
 */

#ifndef USER_DATA_H_
#define USER_DATA_H_

#include "data_format.h"
#include "data_decoder.h"
#include <string>
/// @cond
#include <inttypes.h>
/// @endcond

/**
 * Customized class for storing processed UserFICData_t fields.
 * @copydetails user_data.h
 */
class CFicData {
    public:

        CFicData(); ///< trival default constructor

        /**
         * Sets fields according to given UserFICData_t structure.
         * @param[in] data data to process fill
         */
        void Set(UserFICData_t *data);

        ///Prints valid fields (<tt>\--detailed-info</tt> option).
        void Print(void);

        /**
         * Decodes whole given string from EBU Latin to UTF-8.
         * @param[in] label label to decode
         * @return UTF-8 encoded string
         */
        static std::string DecodeEBULabel(const char label[16]);


        bool DAB_plus_;             ///< differentiates DAB and DAB+ standards
        size_t bitrate_;          ///< current station bitrate in kbps
        uint16_t station_id_;       ///< id of current station
        std::string station_label_; ///< current station label
        UserFICData_t::UTC_t time_; ///< current UTC time
        const char* programme_type_;///< current programe type short description
        std::string coordinates_;   ///< stringified coordinates
        std::list<stationInfo> stations; ///< list of all available stations

    private:

        /**
         * Human-readable coordinate structure.
         */
        struct coordinate_t {
            /// coordinate_t varieties
            enum variety_t {
                LATITUDE,
                LONGITUDE,
            };

            /// coordinate_t magic values
            enum {
                LATITUDE_MULTIPLIER = 90,   ///< 90 degrees
                LATITUDE_POSITIVE = 'N',    ///< North
                LATITUDE_NEGATIVE = 'S',    ///< South
                LONGITUDE_MULTIPLIER = 180, ///< 180 degrees
                LONGITUDE_POSITIVE = 'W',   ///< West
                LONGITUDE_NEGATIVE = 'E',   ///< East
            };

            /**
             * Sets direction_, zeros other fields.
             * @param[in] direction direction
             */
            coordinate_t(char direction);

            char direction_; ///< 'N'/'S' for latitude, 'W'/'E' for longitude
            uint8_t degrees_;
            uint8_t minutes_;
            uint8_t seconds_;
        };

        /**
         * Sets coordinate (latitude or longitude) to human-readable form.
         * @param[in] coarse coarse coordiante value
         * @param[in] variety coordinate_variety_t (LATITUDE or LONGITUDE)
         */
        void SetCoordinate(int16_t coarse, coordinate_t::variety_t variety);

        /// Constructs coordinates_ string based on latitude_ and longitude_.
        void StringifyCoordinates(void);

        coordinate_t latitude_;     ///< transmitter latitude
        coordinate_t longitude_;    ///< transmitter longitude
        /// tells whether primitive fields are valid (see UserFICData_t internal enum)
        uint8_t validity_;

};

#endif /* USER_DATA_H_ */
