/*
 *    Copyright (C) 2018
 *    Matthias P. Braendli (matthias.braendli@mpb.li)
 *
 *    Copyright (C) 2013
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the SDR-J (JSDR).
 *    SDR-J is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    SDR-J is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with SDR-J; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#ifndef __ENERGY_DISPERSAL
#define __ENERGY_DISPERSAL

#include <cstdint>
#include <cstring>
#include <vector>
#include <stdexcept>

class EnergyDispersal {
    public:
        void dedisperse(std::vector<uint8_t>& data)
        {
            if (dispersalVector.size() != data.size()) {
                std::vector<uint8_t> shiftRegister(9, 1);

                dispersalVector.resize(data.size());

                for (size_t i = 0; i < data.size(); i++) {
                    uint8_t b = shiftRegister[8] ^ shiftRegister[4];
                    for (int j = 8; j > 0; j--)
                        shiftRegister[j] = shiftRegister[j - 1];
                    shiftRegister[0] = b;
                    dispersalVector[i] ^= b;
                }
            }

            for (size_t i = 0; i < data.size(); i++) {
                data[i] ^= dispersalVector[i];
            }
        }

    private:
        std::vector<uint8_t> dispersalVector;
};

#endif // __ENERGY_DISPERSAL
