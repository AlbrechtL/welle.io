/**
 * @class SuperFrame
 * @brief Extraction frames, convertion to decimal
 *
 * @author Marcin Trebunia marcintutka94@gmail.com (SuperFrame)
 * @author Jaroslaw Bulat kwant@agh.edu.pl (SuperFrame::CRC16, SuperFrame::BinToDec, SuperFrame::SuperFrameHandle, SuperFrame::SuperframeCircshiftBuff, SuperFrame::FirecodeInit, SuperFrame::FirecodeCheck)
 * @date 7 July 2015 - version 1.0 beta
 * @date 7 July 2016 - version 2.0 beta
 * @date 1 November 2016 - version 2.0
 * @version 2.0
 * @copyright Copyright (c) 2015 Jaroslaw Bulat
 * @copyright Copyright (c) 2016 Jaroslaw Bulat, Marcin Trebunia
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

#include "superframe.h"
#include "reed_solomon.h"
#include <stdint.h>
#include <string.h>
#include <cstddef>
#include <iostream>

using namespace std;

SuperFrame::SuperFrame(ModeParameters *param) :
		        mode_parameters_(param),
		        super_frame_size_(0),
		        superframe_capacity_(8),
		        superframe_cifs_(0),
		        superframe_(NULL),
		        adts_head_idx_(0){
}

SuperFrame::SuperFrame(stationInfo *info, ModeParameters *param) :
		        station_info_(info),
		        mode_parameters_(param),
		        super_frame_size_(0),
		        superframe_capacity_(16),
		        superframe_cifs_(0),
		        superframe_(NULL),
		        adts_head_idx_(0){
                ReedSolomon();
}

SuperFrame::~SuperFrame(){
}

void SuperFrame::SuperFrameHandle(uint8_t *data, uint8_t *write_data){
    size_t bytes_per_cif = msc_info_.number_bits_per_cif/8;
    super_frame_size_ = 0;
    bool superframe = false;
    for( size_t idx=adts_head_idx_; idx<superframe_cifs_; ++idx){
        if( FirecodeCheck(data+idx*bytes_per_cif) ){        // found/verify synchronization!
            adts_head_idx_ = idx;
            if( superframe_cifs_-adts_head_idx_ >= 5 ){
                superframe = true;
                break;
            }else{                                          // not enough data for decoding adts (need 5 CIFs) or header not found
                CircshiftBuff(data);
                return;
            }
        }
    }

    if(!superframe){                                        // not found superframe
        CircshiftBuff(data);
        return;
    }

    uint8_t *adts_frame = data + adts_head_idx_*bytes_per_cif;
    // uint8_t *adts_frameForCustomRS = data + adts_head_idx_*bytes_per_cif;

    ///@todo how many errors should be trigger for 120/110 RS?
    ReedSolomonCorrection(adts_frame, 5*bytes_per_cif);

    // uint32_t i = 0;
    // while(adts_frame[i++] != adts_frameForCustomRS[i++]) {
    //     cout << adts_frame[unsigned(i)] << " " << adts_frameForCustomRS[unsigned(i)] << endl;
    // }

    // decoding ADTS starts here
    uint8_t dac_sbr=(adts_frame[2]&0x60)>>5;    // (18-19) dec_rate & sbr_flag at once
    uint8_t num_aus=0;
    uint8_t adts_dacsbr=0;                      // use last 4 bits
    uint8_t adts_chanconf=0;                    // last 3 bits for channel index
    uint16_t au_start[7];                       // worst case num_aus+1
    memset(au_start,0,7*sizeof(uint16_t));

    switch( dac_sbr ){
    case 0:                                     // 00b, dac=0, sbr=0
        num_aus = 4;                            // number of Access Units (AUs)
        au_start[0] = 8;                        // address of the first AU
        adts_dacsbr = 0x05;                     // [0 1 0 1] freq index for ADTS header
        break;
    case 1:                                     // 01b, dac=0, sbr=1
        num_aus = 2;                            // number of AUs
        au_start[0] = 5;                        // address of the first AU
        adts_dacsbr = 0x08;                     // [1 0 0 0] freq index for ADTS header
        break;
    case 2:                                     // 10b, dac=1, sbr=0
        num_aus = 6;                            // number of AUs
        au_start[0] = 11;                       // address of the first AU
        adts_dacsbr = 0x03;                     // [0 0 1 1] freq index for ADTS header
        break;
    case 3:                                         // 11b, dac=1, sbr=1
        num_aus = 3;                            // number of AUs
        au_start[0] = 6;                        // address of the first AU
        adts_dacsbr = 0x06;                     // [0 1 1 0] freq index for ADTS header
        break;
    }

    //bool aac_channel_mode = (adts_frame[2]&0x10)>>3;  // (20-bit) 0=mono, 1=stereo
    if(adts_frame[2]&0x08){                             // (21-bit) parametric stereo (PS)
        adts_chanconf = 0x2;                            // [ 0 1 0 ] channel index for ADTS header
    } else {
        adts_chanconf = 0x1;                            // [ 0 0 1 ] channel index for ADTS header
    }
    //uint8_t mpeg_surround_config = adts_frame[2]&0x07;

    for( size_t r=1; r<num_aus; ++r)                                // addresses of the AUs from 2:last
        au_start[r] = BinToDec( adts_frame, 24+12*(r-1), 12 );      // start from 25bit, each ADDR has 12bits
    au_start[num_aus] = station_info_->audio_kbps/8*110;

    for( size_t r=0; r<num_aus; ++r)
        if(au_start[r]>=au_start[r+1]){
            adts_head_idx_ = 0;                                     // hard restart superframe handle
            superframe_cifs_ = 0;
            return;
        }

    size_t aac_size = 0;
    size_t crc_errors = 0;
    for( size_t r=0; r<num_aus; ++r ){
        if( !CRC16(adts_frame+au_start[r], au_start[r+1]-au_start[r]) ){
            // przebiegie 1 ; bez RS (RS zmniejsza ilosc danych)
            // z rs moim i istniejacym
            // czas
            crc_errors++;
            continue;
        }

        size_t au_size = au_start[r+1]-au_start[r]-2;               // AU size in bytes, without CRC; -2 bytes?
        size_t adts_size = au_size + 7;                             // frame size = au_size bytes of AU + 7 bytes of the adts-header

        uint8_t adts_header[7];
        memset( adts_header, 0, sizeof(uint8_t)*7 );
        adts_header[0] = 0xFF;                  // (1:12) syncword
        adts_header[1] = 0xF0;
        adts_header[1] |= 0x08;                 // (13) 0=MPEG-4, 1=MPEG-2
        //adts_header[1] &= 0xF9;               // (14-15) 00=MPEG-4, layer=MPEG-2
        adts_header[1] |= 0x01;                 // (16) 1 = no CRC, 0=CRC protection of adst_header is present (+2 bytes in the end)
        //adts_header[2] &= 0x3F;               // (17:18) ? MPEG-4 audio object type minus 1
        adts_header[2] |= adts_dacsbr<<2;       // (19:22) MPEG-4 sampling frequency index (15 not allowed); [ 0 1 1 0] ;
        //adts_header[2] &= 0xFD;               // (23) 1=private stream
        adts_header[2] |= adts_chanconf>>2;     // (24:26) MPEG-4 channel configuration; [ 0 0 1 ]
        adts_header[3] |= adts_chanconf<<6;     // (24:26) MPEG-4 channel configuration; [ 0 0 1 ]
        //adts_header[3] &= 0xDF;               // (27) 1=originality
        //adts_header[3] &= 0xEF;               // (28) 1=home
        adts_header[3] |= 0x08;                 // (29) 1=copyright stream
        adts_header[3] |= 0x04;                 // (30) 1=copywright start
        adts_header[3] |= adts_size>>11;        // (31:43) frame length = 7 bytes of the adts header + au_size bytes of AU
        adts_header[4] = (adts_size>>3)&0xFF;
        adts_header[5] |= (adts_size&0x07)<<5;
        adts_header[5] |= 0x1F;                 // (44:54) unknown yet
        adts_header[6] |= 0xFC;
        //adts_header[6] &= 0x03;               // (55:56) frames count in one packet

        memcpy( write_data+aac_size, adts_header, 7*sizeof(uint8_t) );
        aac_size +=7;
        memcpy( write_data+aac_size, adts_frame+au_start[r], ((au_start[r+1]-au_start[r])-2)*sizeof(uint8_t) );
        aac_size += ((au_start[r+1]-au_start[r])-2)*sizeof(uint8_t);
        super_frame_size_=aac_size;
    }

    // decoded, check next synchro
    if( adts_head_idx_+5 < superframe_cifs_ ){
        if(crc_errors<num_aus)
            adts_head_idx_ +=5;                         // valid next superframe
        else{
            adts_head_idx_+=4;                          // to many CRC erros, mayby synchronization glitch
        }
    }else{                                              // next cif should be beginning of superframe
        adts_head_idx_ = 0;                             // hard restart superframe handle
        superframe_cifs_ = 0;
        return;
    }
    CircshiftBuff(data);
}


void SuperFrame::CircshiftBuff(uint8_t *data){
    if( superframe_cifs_+mode_parameters_->number_of_cif > superframe_capacity_ ){        // next cifs do not fit in buffer
        size_t bytes_per_cif = msc_info_.number_bits_per_cif/8;

        if( (superframe_cifs_+mode_parameters_->number_of_cif)-adts_head_idx_ > superframe_capacity_ ){ // lost synchro, buffer will be overfull
            superframe_cifs_ = 0;
        } else if( adts_head_idx_ < superframe_cifs_){
            memmove( data, data+adts_head_idx_*bytes_per_cif, (superframe_cifs_-adts_head_idx_)*bytes_per_cif );
            superframe_cifs_ -= adts_head_idx_;
        }else{                                                                              // processed all cifs, next cif should be beginning of superframe, not tested!
            superframe_cifs_ = 0;
        }
        adts_head_idx_ = 0;
    }
}

/// Gnuradio + sdr-j implementation of firecode
/// g(x)=(x^11+1)(x^5+x^3+x^2+x+1)=1+x+x^2+x^3+x^5+x^11+x^12+x^13+x^14+x^16
void SuperFrame::FirecodeInit(void){
    uint8_t regs [16];
    uint16_t itab [8];

    for(size_t i=0; i<8; ++i){
        memset(regs, 0, 16*sizeof(uint8_t));
        regs[8+i] = 1;

        uint16_t z;
        for( size_t ii=0; ii<8; ++ii){
            z = regs[15];
            for(size_t jj=15; jj>0; --jj)
                regs[jj] = regs[jj-1] ^ (z & firecode_g_[jj]);
            regs[0] = z;
        }

        uint16_t v = 0;
        for(int8_t ii=15; ii>=0; --ii)
            v = (v << 1) | regs[ii];
        itab[i] = v;

    }
    for(size_t i=0; i<256; ++i){
        firecode_tab_ [i] = 0;
        for(size_t j=0; j<8; ++j){
            if (i & (1 << j))
                firecode_tab_ [i] ^= itab[j];
        }
    }
}


bool SuperFrame::FirecodeCheck(const uint8_t *data){
    uint16_t state = (data[2]<<8) | data[3];
    uint16_t istate;

    for(size_t i=4; i<11; ++i){
        istate = firecode_tab_[state >> 8];
        state = ((istate & 0x00ff) ^ data[i]) | ((istate ^ state << 8) & 0xff00);
    }

    for(size_t i=0; i<2; ++i){
        istate = firecode_tab_[state >> 8];
        state = ((istate & 0x00ff) ^ data[i]) | ((istate ^ state << 8) & 0xff00);
    }

    if( !state )
        return true;
    else
        return false;
}

uint16_t SuperFrame::BinToDec( uint8_t *data, size_t offset, size_t length ){
    uint32_t output = (*(data+offset/8)<<16) | ((*(data+offset/8+1)) << 8) | (*(data+offset/8+2));      // should be big/little endian save
    output >>= 24-length-offset%8;
    output &= (0xFFFF>>(16-length));
    return static_cast<uint16_t>(output);
}

bool SuperFrame::CRC16( uint8_t *data, size_t length ){

    uint16_t CRC = 0xFFFF;
    uint16_t poly= 0x1020;

    *(data+length-2) ^= 0xFF;
    *(data+length-1) ^= 0xFF;

    for( size_t i=0; i<length; ++i ){
        for( size_t b=0; b<8; ++b ){
            if( ((CRC&0x8000)>>15) ^ ((data[i]>>(7-b))&0x0001) ){
                CRC <<=1;
                CRC ^= poly;
                CRC |= 0x0001;
            } else {
                CRC <<=1;
                CRC &=0xFFFE;
            }
        }
    }

    *(data+length-2) ^= 0xFF;
    *(data+length-1) ^= 0xFF;

    if(CRC)
        return false;
    else
        return true;
}
