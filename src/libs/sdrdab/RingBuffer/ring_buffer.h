/**
 * @class RingBuffer
 * @brief implements basic ring buffer capabilities
 *
 * @date Created on: May 13, 2015
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

#include <cstdlib>
#include <sys/types.h>
#include <cstdio>
#include <iostream>
#include <cstddef>
#include <cstring>

#ifndef RINGBUFFER_H_
#define RINGBUFFER_H_

template<typename T>
class RingBuffer {

    public:
        RingBuffer(size_t size);

        virtual ~RingBuffer();

        // shouldn't really be used unless you know if there is enough data
        T ReadNext ();

        // overwrites oldest sample
        void WriteNext (T f);

        // safe methods
        T sReadNext ();
        bool sWriteNext (T f);

        // shouldn't really be used unless you know if there is enough data
        size_t ReadFrom (T *dest_buffer, size_t number_to_write);

        // overwrites oldest data
        size_t WriteInto (T *source_buffer, size_t number_to_write);

        // safe methods (not overwriting/reading)
        size_t sReadFrom (T *dest_buffer, size_t number_to_write);
        size_t sWriteInto (T *source_buffer, size_t number_to_write);

        // returns number of samples stored
        size_t DataStored() const;

        // returns free space left (i.e. possible to write without overwriting unread data)
        size_t FreeSpace() const;

        // calculates Sum value of stored data, doesn't "read" - does not move head/tail pointers
        T Sum();

        // calculates Mean value of stored data, doesn't "read" - does not move head/tail pointers
        T Mean();

        // empties the buffer
        void Reset();

        // initializes whole buffer with f's
        void Initialize(T f);

    protected:
        size_t HeadToRightEnd();
        size_t TailToRightEnd();
        T *buffer;
        size_t head, tail;
        bool last_op_write;
        size_t length;

};

template<typename T>
RingBuffer<T>::RingBuffer(size_t size){
    buffer = new T[size];
    length = size;
    last_op_write = false;
    head = 0;
    tail = 0;
}

template<typename T>
RingBuffer<T>::~RingBuffer(){
    delete[] buffer;
}

template<typename T>
T RingBuffer<T>::ReadNext (){
    T f=*(buffer+tail);
    tail=(tail+1)%length;
    last_op_write = false;
    return f;
}

template<typename T>
void RingBuffer<T>::WriteNext (T f){
    *(buffer+head)=f;
    if (head==tail && last_op_write)
        tail=(tail+1)%length;
    head=(head+1)%length;
    last_op_write = true;
}

template<typename T>
T RingBuffer<T>::sReadNext (){
    if (DataStored())
        return ReadNext();
    else
        return static_cast<T>(0.0);
}

template<typename T>
bool RingBuffer<T>::sWriteNext (T f){
    if (FreeSpace()){
        WriteNext(f);
        return true;
    } else {
        return false;
    }
}

template<typename T>
size_t RingBuffer<T>::ReadFrom (T *dest_buffer, size_t number_to_write){
    last_op_write = false;
    size_t number_written= 0;
    //number_to_write;

    while (number_to_write>TailToRightEnd()){

        memcpy(dest_buffer+number_written,buffer+tail,TailToRightEnd()*sizeof(T));
        number_written+=TailToRightEnd();
        number_to_write-=TailToRightEnd();

        tail=0;
    }
    memcpy(dest_buffer+number_written,buffer+tail,number_to_write*sizeof(T));
    tail+=number_to_write;
    number_written+=number_to_write;
    return number_written;

}

template<typename T>
size_t RingBuffer<T>::WriteInto (T *source_buffer, size_t number_to_write){
    size_t number_written = 0;//number_to_write;

    bool rewrite_tail = false;
    //      printf("%lu\n",FreeSpace());
    if(number_to_write>FreeSpace()){
        //          printf("Rewrite tail\n");
        rewrite_tail = true;
    }
    last_op_write = true;
    while (number_to_write>HeadToRightEnd()){
        memcpy(buffer+head,source_buffer+number_written,HeadToRightEnd()*sizeof(T));
        number_to_write-=HeadToRightEnd();
        number_written+=HeadToRightEnd();
        head=0;
    }
    memcpy(buffer+head,source_buffer+number_written,number_to_write*sizeof(T));


    head+=number_to_write;
    number_written+=number_to_write;
    if (rewrite_tail)
        tail = head;
    return number_written;
}

template<typename T>
size_t RingBuffer<T>::sReadFrom (T *dest_buffer, size_t number_to_write){
    if (number_to_write>DataStored())
        number_to_write=DataStored();
    return ReadFrom(dest_buffer,number_to_write);
}

template<typename T>
size_t RingBuffer<T>::sWriteInto (T *source_buffer, size_t number_to_write){
    if (number_to_write>FreeSpace())
        number_to_write=FreeSpace();
    return WriteInto(source_buffer,number_to_write);
}

template<typename T>
size_t RingBuffer<T>::DataStored() const {
    if (head>tail)
        return head - tail;
    else if (head<tail)
        return length - tail + head;
    else if (last_op_write)
        return length;
    else
        return 0;
}

template<typename T>
size_t RingBuffer<T>::FreeSpace() const {
    return length-DataStored();
}

template<typename T>
size_t RingBuffer<T>::HeadToRightEnd(){
    return length-head;
}

template<typename T>
T RingBuffer<T>::Sum() {

    T sum = static_cast<T>(0.0);
    if (DataStored()==0)
        return sum;

    size_t tail_ind = tail;

    do {
        sum+=*(buffer+tail_ind);
        tail_ind= (tail_ind +1)%length;
    } while (tail_ind!=head);
    return sum;
}

template<typename T>
T RingBuffer<T>::Mean() {
    if (DataStored())
        return Sum()/DataStored();
    else
        return static_cast<T>(0.0);
}

template<typename T>
void RingBuffer<T>::Reset() {
    head = 0;
    tail = 0;
    for (size_t i=0; i<length; i++)
        *(buffer+i)=static_cast<T>(0);
    last_op_write = false;
}

template<typename T>
void RingBuffer<T>::Initialize(T f) {
    head = 0;
    tail = 0;
    for (size_t i=0; i<length; i++)
        *(buffer+i)=f;
    last_op_write = true;

}

template<typename T>
size_t RingBuffer<T>::TailToRightEnd(){
    return length-tail;
}

#endif /* RINGBUFFER_H_ */
