/**
 * @class DePuncturer
 * @brief Depuncturing data
 *
 * @author Marcin Trebunia marcintutka94@gmail.com (DePuncturer)
 * @author Rafal Palej palej@student.agh.edu.pl Jaroslaw Bulat kwant@agh.edu.pl (DePuncturer::DePuncturerProcess, DePuncturer::DePuncturerFICInit, DePuncturer::DePuncturerMSCInit)
 *
 * @date 7 July 2015 - version 1.0 beta
 * @date 7 July 2016 - version 2.0 beta
 * @date 1 November 2016 - version 2.0
 * @version 2.0
 * @copyright Copyright (c) 2015 Rafal Palej
 * @copyright Copyright (c) 2016 Rafal Palej, Marcin Trebunia
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

#include "depuncturer.h"

#include <cstddef>
#include <stdint.h>
#include <stdint.h>
#include <string.h>

DePuncturer::DePuncturer(ModeParameters *param) :
mode_parameters_(param){
}

DePuncturer::DePuncturer(stationInfo *info, ModeParameters *param) :
            		mode_parameters_(param),
					station_info_(info){
}

DePuncturer::~DePuncturer(){
}

void DePuncturer::DePuncturerFICInit(){
	size_t nCif = mode_parameters_->number_of_cif;
	if(mode_parameters_->dab_mode != DAB_MODE_III){
		depunctur_info_.lrange_fic[0] = 2016;    depunctur_info_.lpi_fic[0] = 16;
		depunctur_info_.lrange_fic[1] = 276;     depunctur_info_.lpi_fic[1] = 15;
		depunctur_info_.lrange_fic[2] = 12;      depunctur_info_.lpi_fic[2] = 8;
		depunctur_info_.lrange_fic[3] = 0;       depunctur_info_.lpi_fic[3] = 0;
		depunctur_info_.lrange_fic[4] = 0;       depunctur_info_.lpi_fic[4] = 0;
		depunctur_info_.padding_fic = 0;
		depunctur_info_.audiolen_fic = (depunctur_info_.lrange_fic[0]+depunctur_info_.lrange_fic[1]+depunctur_info_.lrange_fic[2]) * nCif;
	}
	else
	{
		depunctur_info_.lrange_fic[0] = 2784;    depunctur_info_.lpi_fic[0] = 16;
		depunctur_info_.lrange_fic[1] = 276;     depunctur_info_.lpi_fic[1] = 15;
		depunctur_info_.lrange_fic[2] = 12;      depunctur_info_.lpi_fic[2] = 8;
		depunctur_info_.lrange_fic[3] = 0;       depunctur_info_.lpi_fic[3] = 0;
		depunctur_info_.lrange_fic[4] = 0;       depunctur_info_.lpi_fic[4] = 0;
		depunctur_info_.padding_fic = 0;
		depunctur_info_.audiolen_fic = (depunctur_info_.lrange_fic[0]+depunctur_info_.lrange_fic[1]+depunctur_info_.lrange_fic[2]) * nCif;
	}

	depunctur_info_.after_depuncturer_total_len_fic = 0;
	for (size_t i = 0; i < 5; i++)
		depunctur_info_.after_depuncturer_total_len_fic += ((depunctur_info_.lrange_fic[i]) * 32) / (depunctur_info_.lpi_fic[i]+8);
}


void DePuncturer::DePuncturerMSCInit(size_t kbps_sbchsize, uint8_t protection, bool uep, bool isBProtection){
	size_t nCif = mode_parameters_->number_of_cif;
	size_t UEPTAB[5][9];
	if(!uep)
	{
		if(kbps_sbchsize == 32){
			size_t tmpUEPTAB[5][9] = {{3, 4, 17, 0, 5,  3,  2,  0,  0},
					{3, 3, 18, 0, 11, 6,  5,  0,  0},
					{3, 4, 14, 3, 15, 9,  6,  8,  0},
					{3, 4, 14, 3, 22, 13, 8,  13, 0},
					{3, 5, 13, 3, 24, 17, 12, 17, 4}};
			memcpy(UEPTAB, tmpUEPTAB, sizeof(UEPTAB));
		}
		else if(kbps_sbchsize == 48){
			size_t tmpUEPTAB[5][9] = {{4, 3, 26, 3, 5, 4, 2, 3, 0},
					{3, 4, 26, 3, 9, 6, 4, 6, 0},
					{3, 4, 26, 3, 15, 10, 6, 9, 4},
					{3, 4, 26, 3, 24, 14, 8, 15, 0},
					{3, 5, 25, 3, 24, 18, 13, 18, 0}};
			memcpy(UEPTAB, tmpUEPTAB, sizeof(UEPTAB));
		}
		else if(kbps_sbchsize == 56){
			size_t tmpUEPTAB[5][9] = {{6, 10, 23, 3, 5, 4, 2, 3, 0},
					{6, 10, 23, 3, 9, 6, 4, 5, 0},
					{6, 12, 21, 3, 16, 7, 6, 9, 0},
					{6, 10, 23, 3, 23, 13, 8, 13, 8},
					{0, 0, 0, 0, 0, 0, 0, 0, 0}};
			memcpy(UEPTAB, tmpUEPTAB, sizeof(UEPTAB));
		}
		else if(kbps_sbchsize == 64){
			size_t tmpUEPTAB[5][9] = {{6, 9, 31, 2, 5, 3, 2, 3, 0},
					{6, 9, 33, 0, 11, 6, 5, 0, 0},
					{6, 12, 27, 3, 16, 8, 6, 9, 0},
					{6, 10, 29, 3, 23, 13, 8, 13, 8},
					{6, 11, 28, 3, 24, 18, 12, 18, 4}};
			memcpy(UEPTAB, tmpUEPTAB, sizeof(UEPTAB));
		}
		else if(kbps_sbchsize == 80){
			size_t tmpUEPTAB[5][9] = {{6, 10, 41, 3, 6, 3, 2, 3, 0},
					{6, 10, 41, 3, 11, 6, 5, 6, 0},
					{6, 11, 40, 3, 16, 8, 6, 7, 0},
					{6, 10, 41, 3, 23, 13, 8, 13, 8},
					{6, 10, 41, 3, 24, 17, 12, 18, 4}};
			memcpy(UEPTAB, tmpUEPTAB, sizeof(UEPTAB));
		}
		else if(kbps_sbchsize == 96){
			size_t tmpUEPTAB[5][9] = {{7, 9, 53, 3, 5, 4, 2, 4, 0},
					{7, 10, 52, 3, 9, 6, 4, 6, 0},
					{6, 12, 51, 3, 16, 9, 6, 10, 4},
					{6, 10, 53, 3, 22, 12, 9, 12, 0},
					{6, 13, 50, 3, 24, 18, 13, 19, 0}};
			memcpy(UEPTAB, tmpUEPTAB, sizeof(UEPTAB));
		}
		else if(kbps_sbchsize == 112){
			size_t tmpUEPTAB[5][9] = {{6, 13, 50, 3, 24, 18, 13, 19, 0},
					{14, 17, 50, 3, 5, 4, 2, 5, 0},
					{11, 21, 49, 3, 9, 6, 4, 8, 0},
					{11, 23, 47, 3, 16, 8, 6, 9, 0},
					{0, 0, 0, 0, 0, 0, 0, 0, 0}};
			memcpy(UEPTAB, tmpUEPTAB, sizeof(UEPTAB));
		}
		else if(kbps_sbchsize == 128){
			size_t tmpUEPTAB[5][9] = {{12, 19, 62, 3, 5, 3, 2, 4, 0},
					{11, 21, 61, 3, 11, 6, 5, 7, 0},
					{11, 22, 60, 3, 16, 9, 6, 10, 4},
					{11, 21, 61, 3, 22, 12, 9, 14, 0},
					{11, 20, 62, 3, 24, 17, 13, 19, 8}};
			memcpy(UEPTAB, tmpUEPTAB, sizeof(UEPTAB));
		}
		else if(kbps_sbchsize ==  160){
			size_t tmpUEPTAB[5][9] = {{11, 19, 87, 3, 5, 4, 2, 4, 0},
					{11, 23, 83, 3, 11, 6, 5, 9, 0},
					{11, 24, 82, 3, 16, 8, 6, 11, 0},
					{11, 21, 85, 3, 22, 11, 9, 13, 0},
					{11, 22, 84, 3, 24, 18, 12, 19, 0}};
			memcpy(UEPTAB, tmpUEPTAB, sizeof(UEPTAB));
		}
		else if(kbps_sbchsize == 192){
			size_t tmpUEPTAB[5][9] = {{11, 20, 110, 3, 6, 4, 2, 5, 0},
					{11, 22, 108, 3, 10, 6, 4, 9, 0},
					{11, 24, 106, 3, 16, 10, 6, 11, 0},
					{11, 20, 110, 3, 22, 13, 9, 13, 8},
					{11, 21, 109, 3, 24, 20, 13, 24, 0}};
			memcpy(UEPTAB, tmpUEPTAB, sizeof(UEPTAB));
		}
		else if(kbps_sbchsize == 224){
			size_t tmpUEPTAB[5][9] = {{12, 22, 131, 38, 8, 6, 2, 6, 4},
					{12, 26, 127, 3, 12, 8, 4, 11, 0},
					{11, 20, 134, 3, 16, 10, 7, 9, 0},
					{11, 22, 132, 3, 24, 16, 10, 5, 0},
					{11, 24, 130, 3, 24, 20, 12, 20, 4}};
			memcpy(UEPTAB, tmpUEPTAB, sizeof(UEPTAB));
		}
		else if(kbps_sbchsize == 256){
			size_t tmpUEPTAB[5][9] = {{11, 24, 154, 3, 6, 5, 2, 5, 0},
					{11, 24, 154, 3, 12, 9, 5, 10, 4},
					{11, 27, 151, 3, 16, 10, 7, 10, 0},
					{11, 22, 156, 3, 24, 14, 10, 13, 8},
					{11, 26, 152, 3, 24, 19, 14, 18, 4}};
			memcpy(UEPTAB, tmpUEPTAB, sizeof(UEPTAB));
		}
		else if(kbps_sbchsize == 320){
			size_t tmpUEPTAB[5][9] = {{11, 26, 200, 3, 8, 5, 2, 6, 4},
					{11, 25, 201, 3, 13, 9, 5, 10, 8},
					{0, 0, 0, 0, 0, 0, 0, 0, 0},
					{11, 26, 200, 3, 24, 17, 9, 17, 0},
					{0, 0, 0, 0, 0, 0, 0, 0, 0}};
			memcpy(UEPTAB, tmpUEPTAB, sizeof(UEPTAB));
		}
		else if(kbps_sbchsize == 384){
			size_t tmpUEPTAB[5][9] = {{11, 27, 247, 3, 8, 6, 2, 7, 0},
					{0, 0, 0, 0, 0, 0, 0, 0, 0},
					{11, 24, 250, 3, 16, 9, 7, 10, 4},
					{0, 0, 0, 0, 0, 0, 0, 0, 0},
					{12, 28, 245, 3, 24, 20, 14, 23, 8}};
			memcpy(UEPTAB, tmpUEPTAB, sizeof(UEPTAB));

		}
		depunctur_info_.lpi_msc[0] = UEPTAB[5-protection][4];
		depunctur_info_.lpi_msc[1] = UEPTAB[5-protection][5];
		depunctur_info_.lpi_msc[2] = UEPTAB[5-protection][6];
		depunctur_info_.lpi_msc[3] = UEPTAB[5-protection][7];
		depunctur_info_.lpi_msc[4] = 8;

		depunctur_info_.lrange_msc[0] = (UEPTAB[5-protection][0]*128*(depunctur_info_.lpi_msc[0]+8))/32;
		depunctur_info_.lrange_msc[1] = (UEPTAB[5-protection][1]*128*(depunctur_info_.lpi_msc[1]+8))/32;
		depunctur_info_.lrange_msc[2] = (UEPTAB[5-protection][2]*128*(depunctur_info_.lpi_msc[2]+8))/32;
		depunctur_info_.lrange_msc[3] = (UEPTAB[5-protection][3]*128*(depunctur_info_.lpi_msc[3]+8))/32;
		depunctur_info_.lrange_msc[4] = 12;

		depunctur_info_.audiolen_msc = (depunctur_info_.lrange_msc[0]+depunctur_info_.lrange_msc[1]+depunctur_info_.lrange_msc[2]+depunctur_info_.lrange_msc[3]+depunctur_info_.lrange_msc[4]) * nCif;
		depunctur_info_.padding_msc = UEPTAB[5-protection][8];
	}
	else
	{
		size_t n = 0;
		size_t pi1 = 0;
		size_t pi2 = 0;
		size_t len1 = 0;
		size_t len2 = 0;

		if(isBProtection)
		{
			if(protection == 0)
			{
				n = kbps_sbchsize/27;
				pi1 = 10;
				pi2 = 9;
			}
			else if(protection == 1)
			{
				n = kbps_sbchsize/21;
				pi1 = 6;
				pi2 = 5;
			}
			else if(protection == 2)
			{
				n = kbps_sbchsize/18;
				pi1 = 4;
				pi2 = 3;
			}
			else if(protection == 3)
			{
				n = kbps_sbchsize/15;
				pi1 = 2;
				pi2 = 1;
			}
			station_info_->audio_kbps = n * 32;
			len1 = (24*n) - 3;
			len2 = 3;
		}
		else
		{
			if(protection == 0)
			{
				n = kbps_sbchsize/12;
				pi1 = 24;
				pi2 = 23;
				len1 = (6*n) - 3;
				len2 = 3;
			}
			else if(protection == 1)
			{
				n = kbps_sbchsize/8;
				if (n == 1){
					pi1 = 13;
					pi2 = 12;
					len1 = 5;
					len2 = 1;
				}
				else {
					pi1 = 14;
					pi2 = 13;
					len1 = (2*n) - 3;
					len2 = (4*n) + 3;
				}
			}
			else if(protection == 2)
			{
				n = kbps_sbchsize/6;
				pi1 = 8;
				pi2 = 7;
				len1 = (6*n) - 3;
				len2 = 3;
			}
			else if(protection == 3)
			{
				n = kbps_sbchsize/4;
				pi1 = 3;
				pi2 = 2;
				len1 = (4*n) - 3;
				len2 = (2*n) + 3;
			}
			station_info_->audio_kbps = n * 8;
		}

		depunctur_info_.lpi_msc[0] = pi1;
		depunctur_info_.lpi_msc[1] = pi2;
		depunctur_info_.lpi_msc[2] = 8;
		depunctur_info_.lpi_msc[3] = 0;
		depunctur_info_.lpi_msc[4] = 0;

		depunctur_info_.lrange_msc[0] = (len1*128*(pi1+8))/32;
		depunctur_info_.lrange_msc[1] = (len2*128*(pi2+8))/32;
		depunctur_info_.lrange_msc[2] = 12;
		depunctur_info_.lrange_msc[3] = 0;
		depunctur_info_.lrange_msc[4] = 0;

		depunctur_info_.audiolen_msc = (depunctur_info_.lrange_msc[0]+depunctur_info_.lrange_msc[1]+depunctur_info_.lrange_msc[2]+depunctur_info_.lrange_msc[3]+depunctur_info_.lrange_msc[4]) * nCif;
		depunctur_info_.padding_msc = 0;
	}

	depunctur_info_.after_depuncturer_total_len_msc = 0;
	for (size_t i = 0; i < 5; i++)
		depunctur_info_.after_depuncturer_total_len_msc += ((depunctur_info_.lrange_msc[i]) * 32) / (depunctur_info_.lpi_msc[i]+8);
}

void DePuncturer::DePuncturerProcess(const float* data, size_t datalen, float* depunctur, bool msc, bool uep) 
{
	//size_t nCifs = mode_parameters->number_of_cif;

	size_t norginals = 0;
	size_t blocks = 0;
	size_t devpi = 0;

	if (msc) {
		if (uep)
			blocks = 5;
		else
			blocks = 3;
	} else
		blocks = 3;

	for (size_t q = 0; q < blocks; q++) {
		if (msc){
			datalen = depunctur_info_.lrange_msc[q];
			if (depunctur_info_.lpi_msc[q] >= 24)
				devpi = 24;
			else
				devpi = depunctur_info_.lpi_msc[q];
		} else {
			datalen = depunctur_info_.lrange_fic[q];
			if (depunctur_info_.lpi_fic[q] >= 24)
				devpi = 24;
			else
				devpi = depunctur_info_.lpi_fic[q];
		}

		norginals = ((datalen) * 32) / (devpi+8);

		size_t v = devPI_[devpi-1];
		size_t byte_offset = 0;
		for (size_t i = 0; i < norginals; i++) {
			*depunctur = 0;

			if (byte_offset == 32){
				byte_offset = 0;
			}

			if ((((v << byte_offset) >> 31) & 1) == 1) {
				*depunctur = *data;
				data++;
			}
			depunctur++;
			byte_offset++;
		}
	}
}
