/**
 * @class DeViterbi
 * @brief Performs Viterbi decoding
 *
 * @author Marcin Trebunia marcintutka94@gmail.com (DeViterbi)
 * @author Adrian Włosiak adwlosiakh@gmail.com (DeViterbi:DeViterbiProcess 25%)
 * @author Tomasz Zieliński tzielin@agh.edu.pl, Jarosław Bułat kwant@agh.edu.pl (DeViterbi::DeViterbiProcess, DeViterbi::DeViterbiInit)*
 * @date 7 July 2015 - version 1.0 beta
 * @date 7 July 2016 - version 2.0 beta
 * @date 1 November 2016 - version 2.0
 * @version 2.0
 * @copyright Copyright (c) 2015 Jaroslaw Bulat, Tomasz Zielinski,
 * @copyright Copyright (c) 2016 Jaroslaw Bulat, Tomasz Zielinski, Marcin Trebunia
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

#include "deviterbi.h"
#include <stdint.h>
#include <string.h>
#include <cstddef>

#ifdef SSE3_FOUND
#include <x86intrin.h>
#endif

DeViterbi::DeViterbi(){
}


DeViterbi::~DeViterbi(){
}


void DeViterbi::DeViterbiProcess(float* input, size_t size, uint8_t* output) {
    size_t NMemDepth;                       // 5*NDelays
    size_t Pos, mRead, mWrite, pLast;
    float Value1, Value2;
    size_t Ny4 = size/4;                    // 4-time redundancy
    size_t Ny4_8 = Ny4/8;                   // output counter
    size_t index;
    size_t NStreams_3 = NStreams_-3;
    size_t bitShift = 7;

    NMemDepth = 5*NDelays_;                 // 30
    mRead = 0;
    mWrite = NStates_;
    pLast = NMemDepth-1;                    // the last (actual) position in PathBuff
    size_t NMemDepth_1 = NMemDepth-1;

    memset(accMetricBuff_, 0, sizeof(float)*128);   // ToDo: possible optimization  (not known if all need to be set to 0)
    zeroTail_[0] = input[size-4];           // copy tail of data to beginning of axu buffer.
    zeroTail_[1] = input[size-3];
    zeroTail_[2] = input[size-2];
    zeroTail_[3] = input[size-1];

    size_t outputCounter = 0;
    for(size_t i=0; i<Ny4+NMemDepth; ++i){    // for each input 4-element symbol (one row with 4 redundant values)
        int *inStat1_index = inStat1_;
        int *inStat2_index = inStat2_;
        int *refOut1tmp = refOut1_;
        int *refOut2tmp = refOut2_;
        float *yIntmp = input+i*NStreams_;

        if( i>=Ny4)
            yIntmp = zeroTail_;
        //else  //ToDo: possible optimization, Value1==Value2==0 because yIntmp==0

#ifdef SSE3_FOUND
        //sign mask
        const __m128 _m_signMask = _mm_set1_ps(-0.0f); // broadcast 32bit number with 1 on sign bit to all elements of register
        //load input
        __m128 _m_inp = _mm_set_ps(yIntmp[3], yIntmp[2], yIntmp[1], yIntmp[0]);
#endif

        for(index=0; index<NStates_; ++index ){

#ifdef SSE3_FOUND
            __m128 _m_aMB1, _m_aMB2;
            float accMetricBuff_toWrite;
            int inStat_index; //temporaries seem to hint compiler to use cache more efficiently

            //load refOuts
            __m128 _m_rO1 = _mm_castsi128_ps(_mm_set_epi32(refOut1tmp[3], refOut1tmp[2], refOut1tmp[1], refOut1tmp[0]));
            __m128 _m_rO2 = _mm_castsi128_ps(_mm_set_epi32(refOut2tmp[3], refOut2tmp[2], refOut2tmp[1], refOut2tmp[0]));
            //mask
            __m128 _m_signs1 = _mm_and_ps(_m_rO1, _m_signMask);
            __m128 _m_signs2 = _mm_and_ps(_m_rO2, _m_signMask);
            //load accMetricBuff_
            _m_aMB1 = _mm_set1_ps(accMetricBuff_[*inStat1_index + mRead]);
            _m_aMB2 = _mm_set1_ps(accMetricBuff_[*inStat2_index + mRead]);
            //sign multiplication
            __m128 _m_vm1 = _mm_xor_ps(_m_signs1, _m_inp);
            __m128 _m_vm2 = _mm_xor_ps(_m_signs2, _m_inp);
            //horizontal add
            __m128 _m_vshuf1 = _mm_movehdup_ps(_m_vm1); // a b c d -> b b d d
            __m128 _m_vshuf2 = _mm_movehdup_ps(_m_vm2);
            __m128 _m_val1 = _mm_add_ps(_m_vm1, _m_vshuf1); // a+b b+b c+d d+d
            __m128 _m_val2 = _mm_add_ps(_m_vm2, _m_vshuf2);
            _m_vshuf1 = _mm_movehl_ps(_m_vshuf1, _m_val1); // c+d d+d d d
            _m_vshuf2 = _mm_movehl_ps(_m_vshuf2, _m_val2);
            _m_val1 = _mm_add_ss(_m_val1, _m_vshuf1); // (a+b)+(c+d) b+b c+d d+d
            _m_val2 = _mm_add_ss(_m_val2, _m_vshuf2);
            _m_val1 = _mm_add_ss(_m_val1, _m_aMB1); //add accMetricBuff
            _m_val2 = _mm_add_ss(_m_val2, _m_aMB2);

            Value1 = _mm_cvtss_f32(_m_val1); // a+b+c+d
            Value2 = _mm_cvtss_f32(_m_val2);

            // Decision: what transition path is better?
            if( Value1 > Value2 ){  // EVEN path; perf top indicates that this line is bottleneck (gcc vectorizes this though)
                accMetricBuff_toWrite = Value1;
                inStat_index = *inStat1_index;
            }
            else                   // EVEN path
            {
                accMetricBuff_toWrite = Value2;
                inStat_index = *inStat2_index;
            }
            pathBuff_[index][pLast] = inStat_index;
            accMetricBuff_[index+mWrite] = accMetricBuff_toWrite;

            refOut1tmp += 4;
            refOut2tmp += 4;
#else
            // for each inner state 0:63
            if( *refOut1tmp++>0 )   Value1 = *yIntmp;               // fancy vector multiplication (speedUP)
            else                    Value1 = -*yIntmp;
            if( *refOut2tmp++>0 )   Value2 = *yIntmp++;
            else                    Value2 = -*yIntmp++;

            if( *refOut1tmp++>0 )   Value1 += *yIntmp;
            else                    Value1 -= *yIntmp;
            if( *refOut2tmp++>0 )   Value2 += *yIntmp++;
            else                    Value2 -= *yIntmp++;

            if( *refOut1tmp++>0 )   Value1 += *yIntmp;
            else                    Value1 -= *yIntmp;
            if( *refOut2tmp++>0 )   Value2 += *yIntmp++;
            else                    Value2 -= *yIntmp++;

            if( *refOut1tmp>0 )     Value1 += *yIntmp + accMetricBuff_[ *inStat1_index+mRead ];
            else                    Value1 -= *yIntmp - accMetricBuff_[ *inStat1_index+mRead ];
            if( *refOut2tmp>0 )     Value2 += *yIntmp + accMetricBuff_[ *inStat2_index+mRead ];
            else                    Value2 -= *yIntmp - accMetricBuff_[ *inStat2_index+mRead ];

            refOut1tmp +=  NStreams_3;
            refOut2tmp +=  NStreams_3;
            yIntmp -= 3;

            // Decision: what transition path is better?
            if( Value1 > Value2 ){  // EVEN path
                accMetricBuff_[index+mWrite] = Value1;
                pathBuff_[index][pLast] = *inStat1_index;
            }
            else                   // EVEN path
            {
                accMetricBuff_[index+mWrite] = Value2;
                pathBuff_[index][pLast] = *inStat2_index;
            }
#endif

            inStat1_index++;
            inStat2_index++;
        }

        // DECISION
        if( i > NMemDepth_1 ) {
            float *accMetricBufftmp = accMetricBuff_ + index+mWrite;
            Pos = 0;
            float posmax = *accMetricBufftmp;
            for(size_t x=1; x<NStates_; ++x ){
                if(*accMetricBufftmp > posmax){
                    posmax = *accMetricBufftmp;
                    Pos = x;
                }
                accMetricBufftmp++;
            }

            int pLast_NMemDepth = pLast+NMemDepth;
            for(size_t p=0; p<NMemDepth; ++p){                        // trace path back
                if(pLast<p) {
                    Pos = pathBuff_[Pos][pLast_NMemDepth-p];           // read Pos for time "k-1"
                } else {
                    Pos = pathBuff_[Pos][pLast-p];                     // read Pos for time "k-1"
                }
            }

            if(7==bitShift)                     // clear output bytes
                *output = 0;

            if (Pos > NStates2_1_) {            // add ,,1''
                *output |= 1<<bitShift;
            }

            if(!bitShift){                      // increment pointer to output buffer (next byte)
                outputCounter++;
                if( outputCounter >= Ny4_8 ){   // reach last byte
                    return;
                }
                output++;
                bitShift=7;
            } else {
                bitShift--;
            }
        }

        // BUFFER CONTROL
        pLast++;
        if( pLast > NMemDepth_1)
            pLast=0;

        if( mRead==0 ) {
            mRead=NStates_;
            mWrite=0;
        } else {
            mRead=0;
            mWrite=NStates_;
        }
    }
}


void DeViterbi::DeViterbiInit(void) {
    NStreams_=4;
    NTaps_=7;
    NDelays_=6;
    NStates_=64;
    NStates2_=NStates_/2;
    NStates2_1_ = NStates_/2-1;

    int ii,iip;
    int addr;

    // Initialization of InState1 and InState2 matrices
    for(size_t i=0; i<NStates2_; i++)        // 0,1,2,...,31
    {
        ii = 2*i;
        inStat1_[i]=ii;
        inStat1_[i+NStates2_]=ii;               // InStates1 = [ 0,2,4,...,62,  0,2,4,...,62]
        iip=ii+1;
        inStat2_[i]=iip;
        inStat2_[i+NStates2_]=iip;          // InStates2 = [ 1,3,5,...,63,  1,3,5,...,63]
    }

    // Initialization of RefOut1 and RefOut2 matrices
    int NewBit;
    for(size_t i=0; i<NStates_ ;i++)         // for 0,1,2,3,...,63
    {
        ii=i*NStreams_;
        if(i<NStates2_){
            NewBit=0;   // for 0,1,2,..., 31
        }
        else{
            NewBit=1;   // for 32,33,34,...,63
        }
        for(size_t k=0; k<NStreams_; k++)     // for 0,1,2,3
        {
            addr = k*NTaps_;   // address of the first element of the next rows of Generator

            const int *in1 = nextBits_ + NDelays_*inStat1_[i];
            const int *in2 = gen_ + addr;
            int acc = NewBit*in2[0];
            for(size_t x=1; x<NTaps_; ++x){
                acc += in1[x-1]*in2[x];
            }
            refOut1_[ii+k] = 1 - 2*( acc % 2); // multiply two NTaps=7-element vectors

            in1 = nextBits_ + NDelays_*inStat2_[i];
            in2 = gen_ + addr;
            acc = NewBit*in2[0];
            for(size_t x=1; x<NTaps_; ++x){
                acc += in1[x-1]*in2[x];
            }
            refOut2_[ii+k] = 1 - 2*( acc % 2);  // multiply two NTaps=7-element vectors
        }
    }

    memset( zeroTail_, 0, sizeof(float)*120 );                  // zero tail initialization
}