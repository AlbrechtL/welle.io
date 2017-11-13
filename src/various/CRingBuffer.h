/**
* @class CRingBuffer
* @brief implements basic ring buffer capabilities

*    Copyright (C) 2017
*    Albrecht Lohofener (albrechtloh@gmx.de)
*
*    This file is based on SDRDAB
* @date Created on: May 13, 2015
* @author: Paweł Szulc <pawel_szulc@onet.pl>
* @date 7 July 2015 - version 1.0 beta
* @date 7 July 2016 - version 2.0 beta
* @date 1 November 2016 - version 2.0
* @version 2.0
* @copyright Copyright (c) 2015 Paweł Szul
*
*    This file is part of the welle.io.
*    Many of the ideas as implemented in welle.io are derived from
*    other work, made available through the GNU general Public License.
*    All copyrights of the original authors are recognized.
*
*    welle.io is free software; you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation; either version 2 of the License, or
*    (at your option) any later version.
*
*    welle.io is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    welle.io is free software; you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation; either version 2 of the License, or
*    (at your option) any later version.
*
*    welle.io is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with welle.io; if not, write to the Free Software
*    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*/

#include <cstdlib>
#include <sys/types.h>
#include <cstdio>
#include <iostream>
#include <cstddef>
#include <cstring>
#include <mutex>

#ifndef CRINGBUFFER_H_
#define CRINGBUFFER_H_

template<typename T>
class CRingBuffer {

    public:
        CRingBuffer(size_t size);
        virtual ~CRingBuffer();

        // safe methods (not overwriting/reading)
        size_t getDataFromBuffer (T *dest_buffer, size_t number_to_write);
        size_t putDataIntoBuffer (T *source_buffer, size_t number_to_write);

        // returns number of samples stored
        size_t GetRingBufferReadAvailable();

        // returns free space left (i.e. possible to write without overwriting unread data)
        size_t FreeSpace();

        // empties the buffer
        void FlushRingBuffer();

    private:
        // safe methods
        T sReadNext ();
        bool sWriteNext (T f);

        // shouldn't really be used unless you know if there is enough data
        T ReadNext ();

        // overwrites oldest sample
        void WriteNext (T f);

        // shouldn't really be used unless you know if there is enough data
        size_t ReadFrom (T *dest_buffer, size_t number_to_write);

        // overwrites oldest data
        size_t WriteInto (T *source_buffer, size_t number_to_write);


        // initializes whole buffer with f's
        void Initialize(T f);

        size_t HeadToRightEnd();
        size_t TailToRightEnd();
        T *buffer;
        size_t head, tail;
        bool last_op_write;
        size_t length;

        std::mutex work_lock;
};

template<typename T>
CRingBuffer<T>::CRingBuffer(size_t size){
    buffer = new T[size];
    length = size;
    last_op_write = false;
    head = 0;
    tail = 0;
}

template<typename T>
CRingBuffer<T>::~CRingBuffer(){
    delete[] buffer;
}

template<typename T>
T CRingBuffer<T>::ReadNext (){
    T f=*(buffer+tail);
    tail=(tail+1)%length;
    last_op_write = false;
    return f;
}

template<typename T>
void CRingBuffer<T>::WriteNext (T f){
    *(buffer+head)=f;
    if (head==tail && last_op_write)
        tail=(tail+1)%length;
    head=(head+1)%length;
    last_op_write = true;
}

template<typename T>
T CRingBuffer<T>::sReadNext (){
    if (GetRingBufferReadAvailable())
        return ReadNext();
    else
        return static_cast<T>(0.0);
}

template<typename T>
bool CRingBuffer<T>::sWriteNext (T f){
    if (FreeSpace()){
        WriteNext(f);
        return true;
    } else {
        return false;
    }
}

template<typename T>
size_t CRingBuffer<T>::ReadFrom (T *dest_buffer, size_t number_to_write){
    last_op_write = false;
    size_t number_written= 0;
    //number_to_write;

    std::lock_guard<std::mutex> guard(work_lock);

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
size_t CRingBuffer<T>::WriteInto (T *source_buffer, size_t number_to_write){
    size_t number_written = 0;//number_to_write;

    std::lock_guard<std::mutex> guard(work_lock);

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
size_t CRingBuffer<T>::getDataFromBuffer (T *dest_buffer, size_t number_to_write){
    if (number_to_write>GetRingBufferReadAvailable())
        number_to_write=GetRingBufferReadAvailable();
    return ReadFrom(dest_buffer,number_to_write);
}

template<typename T>
size_t CRingBuffer<T>::putDataIntoBuffer (T *source_buffer, size_t number_to_write){
    if (number_to_write>FreeSpace())
        number_to_write=FreeSpace();
    return WriteInto(source_buffer,number_to_write);
}

template<typename T>
size_t CRingBuffer<T>::GetRingBufferReadAvailable() {
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
size_t CRingBuffer<T>::FreeSpace() {
    return length-GetRingBufferReadAvailable();
}

template<typename T>
size_t CRingBuffer<T>::HeadToRightEnd(){
    return length-head;
}

template<typename T>
void CRingBuffer<T>::FlushRingBuffer() {
    work_lock.lock();

    head = 0;
    tail = 0;
    for (size_t i=0; i<length; i++)
        *(buffer+i)=static_cast<T>(0);
    last_op_write = false;

    work_lock.unlock();
}

template<typename T>
void CRingBuffer<T>::Initialize(T f) {
    head = 0;
    tail = 0;
    for (size_t i=0; i<length; i++)
        *(buffer+i)=f;
    last_op_write = true;

}

template<typename T>
size_t CRingBuffer<T>::TailToRightEnd(){
    return length-tail;
}

#endif /* CRINGBUFFER_H_ */
