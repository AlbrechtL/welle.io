/**
 * @file data_decoder_data.h
 * @brief tables
 *
 * fixed data (from standards) necessary for DataDecoder
 *
 * @author Jaroslaw Bulat kwant@agh.edu.pl (Viterbi)
 * @author Tomasz Zieliński tzielin@agh.edu.pl (Viterbi)
 * @date 7 July 2015 - version 1.0 beta
 * @date 7 July 2016 - version 2.0 beta
 * @date 1 November 2016 - version 2.0
 * @version 2.0
 * @copyright Copyright (c) 2015 Jaroslaw Bulat, Tomasz Zieliński.
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

#ifndef DATADECODERDATA_H_
#define DATADECODERDATA_H_

const size_t DePuncturer::tmpDevPI_[24] = {
    3364391048u,
    3364407432u,
    3368601736u,
    3368601800u,
    3435710664u,
    3435711688u,
    3435973832u,
    3435973836u,
    3972844748u,
    3972852940u,
    3974950092u,
    3974950124u,
    4008504556u,
    4008505068u,
    4008636140u,
    4008636142u,
    4277071598u,
    4277075694u,
    4278124270u,
    4278124286u,
    4294901502u,
    4294901758u,
    4294967294u,
    4294967295u
};

const int ExtractFromBitstream::Table6SubChannelShortInfo_[64][4] = {
    { 0, 16, 5, 32 },
    { 1, 21, 4, 32 },
    { 2, 24, 3, 32 },
    { 3, 29, 2, 32 },
    { 4, 35, 1, 32 },
    { 5, 24, 5, 48 },
    { 6, 29, 4, 48 },
    { 7, 35, 3, 48 },
    { 8, 42, 2, 48 },
    { 9, 52, 1, 48 },
    { 10, 29, 5, 56 },
    { 11, 35, 4, 56 },
    { 12, 42, 3, 56 },
    { 13, 52, 2, 56 },
    { 14, 32, 5, 64 },
    { 15, 42, 4, 64 },
    { 16, 48, 3, 64 },
    { 17, 58, 2, 64 },
    { 18, 70, 1, 64 },
    { 19, 40, 5, 80 },
    { 20, 52, 4, 80 },
    { 21, 58, 3, 80 },
    { 22, 70, 2, 80 },
    { 23, 84, 1, 80 },
    { 24, 48, 5, 96 },
    { 25, 58, 4, 96 },
    { 26, 70, 3, 96 },
    { 27, 84, 2, 96 },
    { 28, 104, 1, 96 },
    { 29, 58, 5, 112 },
    { 30, 70, 4, 112 },
    { 31, 84, 3, 112 },
    { 32, 104, 2, 112 },
    { 33, 64, 5, 128 },
    { 34, 84, 4, 128 },
    { 35, 96, 3, 128 },
    { 36, 116, 2, 128 },
    { 37, 140, 1, 128 },
    { 38, 80, 5, 160 },
    { 39, 104, 4, 160 },
    { 40, 116, 3, 160 },
    { 41, 140, 2, 160 },
    { 42, 168, 1, 160 },
    { 43, 96, 5, 192 },
    { 44, 116, 4, 192 },
    { 45, 140, 3, 192 },
    { 46, 168, 2, 192 },
    { 47, 208, 1, 192 },
    { 48, 116, 5, 224 },
    { 49, 140, 4, 224 },
    { 50, 168, 3, 224 },
    { 51, 208, 2, 224 },
    { 52, 232, 1, 224 },
    { 53, 128, 5, 256 },
    { 54, 168, 4, 256 },
    { 55, 192, 3, 256 },
    { 56, 232, 2, 256 },
    { 57, 280, 1, 256 },
    { 58, 160, 5, 320 },
    { 59, 208, 4, 320 },
    { 60, 280, 2, 320 },
    { 61, 192, 5, 384 },
    { 62, 280, 3, 384 },
    { 63, 416, 1, 384 },
};
/**
 * Structures used to avoid a duplication of stations
 */
struct SubChannel_Basic_Sort
{
    bool operator()(MCI::SubChannel_Basic_Information  const subid1, MCI::SubChannel_Basic_Information  const subid2)
    {
        return subid1.subchannel_id < subid2.subchannel_id;
    }
};

struct SubChannel_Basic_Unique
{
    bool operator()(MCI::SubChannel_Basic_Information  const subid1, MCI::SubChannel_Basic_Information  const subid2)
    {
        return subid1.subchannel_id == subid2.subchannel_id;
    }
};

const int DeViterbi::gen_[28] = {1,0,1,1,0,1,1,1,1,1,1,0,0,1,1,1,0,0,1,0,1,1,0,1,1,0,1,1};
const int DeViterbi::nextBits_[384]={ 0,0,0,0,0,0,
    0,0,0,0,0,1,
    0,0,0,0,1,0,
    0,0,0,0,1,1,
    0,0,0,1,0,0,
    0,0,0,1,0,1,
    0,0,0,1,1,0,
    0,0,0,1,1,1,
    0,0,1,0,0,0,
    0,0,1,0,0,1,
    0,0,1,0,1,0,
    0,0,1,0,1,1,
    0,0,1,1,0,0,
    0,0,1,1,0,1,
    0,0,1,1,1,0,
    0,0,1,1,1,1,
    0,1,0,0,0,0,
    0,1,0,0,0,1,
    0,1,0,0,1,0,
    0,1,0,0,1,1,
    0,1,0,1,0,0,
    0,1,0,1,0,1,
    0,1,0,1,1,0,
    0,1,0,1,1,1,
    0,1,1,0,0,0,
    0,1,1,0,0,1,
    0,1,1,0,1,0,
    0,1,1,0,1,1,
    0,1,1,1,0,0,
    0,1,1,1,0,1,
    0,1,1,1,1,0,
    0,1,1,1,1,1,
    1,0,0,0,0,0,
    1,0,0,0,0,1,
    1,0,0,0,1,0,
    1,0,0,0,1,1,
    1,0,0,1,0,0,
    1,0,0,1,0,1,
    1,0,0,1,1,0,
    1,0,0,1,1,1,
    1,0,1,0,0,0,
    1,0,1,0,0,1,
    1,0,1,0,1,0,
    1,0,1,0,1,1,
    1,0,1,1,0,0,
    1,0,1,1,0,1,
    1,0,1,1,1,0,
    1,0,1,1,1,1,
    1,1,0,0,0,0,
    1,1,0,0,0,1,
    1,1,0,0,1,0,
    1,1,0,0,1,1,
    1,1,0,1,0,0,
    1,1,0,1,0,1,
    1,1,0,1,1,0,
    1,1,0,1,1,1,
    1,1,1,0,0,0,
    1,1,1,0,0,1,
    1,1,1,0,1,0,
    1,1,1,0,1,1,
    1,1,1,1,0,0,
    1,1,1,1,0,1,
    1,1,1,1,1,0,
    1,1,1,1,1,1 };

const uint8_t SuperFrame::firecode_g_[16]={1,1,1,1,0,1,0,0,0,0,0,1,1,1,1,0};

const size_t DataDecoder::pos_table_[16] = {15, 7, 11, 3, 13, 5, 9, 1, 14, 6, 10, 2, 12, 4, 8, 0};
const char* DataDecoder::InternationalProgrammeTable_[32][8]={
    "None",
    "News",
    "Affairs",
    "Info",
    "Sport",
    "Educate",
    "Drama",
    "Arts",
    "Science",
    "Talk",
    "Pop",
    "Rock",
    "Easy",
    "Classics",
    "Classics",
    "Other_M",
    "Weather",
    "Finance",
    "Children",
    "Factual",
    "Religion",
    "Phone_In",
    "Travel",
    "Leisure",
    "Jazz",
    "Country",
    "Nation_M",
    "Oldies",
    "Folk",
    "Document"
};

const char* DataDecoder::InternationalProgrammeTablePolish_[32][8]={
    "Brak",
    "Wiadom.",
    "Biezace",
    "Inform.",
    "Sport",
    "Edukacja",
    "Dramat",
    "Kultura",
    "Nauka",
    "Rozne",
    "Pop",
    "Rock",
    "M.Lekka",
    "Lek.Klas",
    "Pow.Klas",
    "Muz.Inna",
    "Pogoda",
    "Finanse",
    "Dzieci",
    "Spoleczn",
    "Religia",
    "Telefon",
    "Podroze",
    "CzasWol.",
    "Jazz",
    "Country",
    "M.Narod.",
    "M._Arch.",
    "M.Ludowa",
    "Dokument"
};

#endif
