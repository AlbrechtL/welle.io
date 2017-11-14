/**
 * @class UserData
 * @brief UserData implementation
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

#include "CFicData.h"
#include <cmath>
/// @cond
#include <cstdio>
#include <cstring>
/// @endcond

CFicData::CFicData()
    : DAB_plus_(),
    bitrate_(),
    station_id_(0),
    station_label_(std::string("Not Available")),
    time_(),
    programme_type_(DataDecoder::InternationalProgrammeTable_[0][0]),
    coordinates_(),
    stations(),
    latitude_('N'),
    longitude_('E'),
    validity_(0){
        StringifyCoordinates();
    }

CFicData::coordinate_t::coordinate_t(char direction)
    : direction_(direction),
    degrees_(0),
    minutes_(0),
    seconds_(0) {
    }

void CFicData::Set(UserFICData_t *data) {

    if(data != NULL)
    {
        this->DAB_plus_ = data->DAB_plus_;
        this->bitrate_ = data->bitrate_;

        if ((data->validity_ & UserFICData_t::LABEL_VALID) &&
                this->station_id_ != data->service_id_)
        {
            this->station_id_ = data->service_id_;
            this->station_label_ = data->service_label_;
            this->validity_ |= UserFICData_t::LABEL_VALID;
        }

        if ((data->validity_ & UserFICData_t::PROGRAMME_TYPE_VALID) &&
                data->programme_type_ < 32)
        {
            this->programme_type_ =
                DataDecoder::InternationalProgrammeTable_[data->programme_type_][0];
            this->validity_ |= UserFICData_t::PROGRAMME_TYPE_VALID;
        }

        if (data->validity_ & UserFICData_t::TIME_VALID) {
            memcpy(static_cast<void *>(&this->time_),
                    static_cast<void *>(&data->time_),
                    sizeof(UserFICData_t::UTC_t));
            this->validity_ |= UserFICData_t::TIME_VALID;
        }

        if (data->validity_ & UserFICData_t::COORDINATES_VALID) {
            this->SetCoordinate(data->latitude_,
                    CFicData::coordinate_t::LATITUDE);
            this->SetCoordinate(data->longitude_,
                    CFicData::coordinate_t::LONGITUDE);
            this->StringifyCoordinates();
            this->validity_ |= UserFICData_t::COORDINATES_VALID;
        }

        this->stations.clear();
        this->stations = data->stations;

        ///@todo store information about image
    }
}
/*
void CFICData::Print(void) {
    if (!this->validity_)
        return;

    if (this->validity_ & UserFICData_t::LABEL_VALID) {
        ReadLine::printf(" Station: %s\n", this->station_label_.c_str());
    }

    if (this->bitrate_ > 0)
        ReadLine::printf(" Standard: %s; bitrate: %zu kbps\n",
                (this->DAB_plus_ ? "DAB+" : "DAB"),
                this->bitrate_);
    else
        ReadLine::printf(" Standard: %s\n", (this->DAB_plus_ ? "DAB+" : "DAB"));

    if (this->validity_ & UserFICData_t::PROGRAMME_TYPE_VALID) {
        ReadLine::printf(" Programme type: %s\n", this->programme_type_);
    }

    if (this->validity_ & UserFICData_t::COORDINATES_VALID) {
        ReadLine::printf(" Transmitter coordinates: %s\n",
                this->coordinates_.c_str());
    }

    if (this->validity_ & UserFICData_t::TIME_VALID) {
        ReadLine::printf(" UTC time: %.2hhu:%.2hhu:%.2hhu.%.4hu\n",
                this->time_.h_,
                this->time_.m_,
                this->time_.s_,
                this->time_.ms_);
    }
}
*/
std::string CFicData::DecodeEBULabel(const char label[16])
{
    std::string retval;
    retval.reserve(16+8);

    for (int i = 0; i < 16; i++) {
        retval.append( DataDecoder::EBULatinToUTF8(label[i]) );
        if (label[i] == '\0')
            break;
    }
    return retval;
}

void CFicData::SetCoordinate(int16_t coarse, coordinate_t::variety_t variety)
{
    coordinate_t CFicData::*coord;
    double multiplier;
    if (variety == coordinate_t::LATITUDE) {
        coord = &CFicData::latitude_;
        multiplier = coordinate_t::LATITUDE_MULTIPLIER;
        if (coarse < 0) {
            (this->*coord).direction_ = coordinate_t::LATITUDE_NEGATIVE;
            coarse *= -1;
        } else {
            (this->*coord).direction_ = coordinate_t::LATITUDE_POSITIVE;
        }
    } else if (variety == coordinate_t::LONGITUDE) {
        coord = &CFicData::longitude_;
        multiplier = coordinate_t::LONGITUDE_MULTIPLIER;
        if (coarse < 0) {
            (this->*coord).direction_ = coordinate_t::LONGITUDE_NEGATIVE;
            coarse *= -1;
        } else {
            (this->*coord).direction_ = coordinate_t::LONGITUDE_POSITIVE;
        }
    } else {
        return;
    }

    double intpart, fractpart;
    fractpart = modf( (static_cast<double>(coarse) * multiplier / 32768.f),
            &intpart );
    (this->*coord).degrees_ = static_cast<uint8_t>(intpart);

    fractpart *= 60.f;
    fractpart = modf(fractpart, &intpart);
    (this->*coord).minutes_ = static_cast<uint8_t>(intpart);

    fractpart *= 60.f;
    (this->*coord).seconds_ = static_cast<uint8_t>(fractpart);
}

void CFicData::StringifyCoordinates(void) {
    this->coordinates_.clear();
    unsigned char degree_symbol[3] = { 0xC2, 0xB0, 0x00 };
    char buf[48];
    snprintf(buf, 48, "%hhu%s%hhu'%hhu\"%c %hhu%s%hhu'%hhu\"%c",
            this->latitude_.degrees_,
            degree_symbol,
            this->latitude_.minutes_,
            this->latitude_.seconds_,
            this->latitude_.direction_,
            this->longitude_.degrees_,
            degree_symbol,
            this->longitude_.minutes_,
            this->longitude_.seconds_,
            this->longitude_.direction_);
    this->coordinates_ = std::string(buf);

}
