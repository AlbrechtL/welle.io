/*
 * @author: Paweł Szulc <pawel_szulc@onet.pl>
 * @date 7 July 2015 - version 1.0 beta
 * @date 7 July 2016 - version 2.0 beta
 * @date 1 November 2016 - version 2.0
 * @version 2.0
 * @copyright Copyright (c) 2015 Paweł Szulc
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


#include "resampling_ring_buffer.h"

ResamplingRingBuffer::ResamplingRingBuffer(int quality, size_t size, int channels) : RingBuffer(size){
    resampler = new Resampler(quality, channels);
}

ResamplingRingBuffer::~ResamplingRingBuffer(){
    delete resampler;
}

size_t ResamplingRingBuffer::WriteResampledInto(float *source_buffer,size_t number_to_read, float ratio){

    if (ratio==1.0)
        return sWriteInto(source_buffer,number_to_read);

    size_t number_written = 0;

    size_t max_number_to_write = HeadToRightEnd();
    if (FreeSpace()<max_number_to_write)
        max_number_to_write = FreeSpace();

    last_op_write = true;

    //  printf("Resampling with ratio: %1.8f\n",ratio);
    resampler->SetSourceBuffer(source_buffer,number_to_read);

    int32_t returned = 0;
    do {
        returned = resampler->Resample(buffer+head,max_number_to_write,ratio);
        if (returned != 0){
            number_written+=returned;
            head = (head + returned)%length;
            max_number_to_write = FreeSpace();
            if (max_number_to_write>HeadToRightEnd())
                max_number_to_write=HeadToRightEnd();
        }

    } while ( returned != 0);

    return number_written;

}

//size_t ResamplingRingBuffer::sWriteResampledInto(float *source_buffer,size_t number_to_read, float ratio){
//
//
//}


