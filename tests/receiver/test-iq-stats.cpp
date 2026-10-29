#include "iq_write_stats.h"
#include "ringbuffer.h"
#include <cassert>
#include <vector>
#include <iostream>
int main(){
 RingBuffer<uint8_t> b(1048576);IQWriteStats s;
 std::vector<uint8_t> v(65536,42),out(65536);
 for(int i=0;i<17;++i)s.write(b,v.data(),v.size());
 assert(s.offered==1114112 && s.accepted==1048576 && s.dropped==65536);
 assert(b.getDataFromBuffer(out.data(),out.size())==65536);assert(out==v);
 s.write(b,v.data(),v.size());assert(s.accepted==1114112 && s.dropped==65536);
 RingBuffer<uint8_t> small(8);IQWriteStats p;
 assert(p.write(small,v.data(),6)==6);assert(p.write(small,v.data(),6)==2);
 assert(p.offered==12 && p.accepted==8 && p.dropped==4);
 std::cout<<"PASS: full, partial, recovery; actual RingBuffer and production write wrapper\n";
}
