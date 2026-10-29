#include "iq_transfer.h"
#include "ringbuffer.h"
#include <cassert>
#include <vector>
#include <iostream>
int main(){
 RingBuffer<uint8_t> source(1<<23),dest(1<<20);IQWriteStats stats;
 std::vector<uint8_t> input(3<<20),scratch(65536),output;
 for(size_t i=0;i<input.size();++i)input[i]=(i/2)%251;
 assert(source.putDataIntoBuffer(input.data(),input.size())==(int)input.size());
 for(int i=0;i<16;++i)assert(transferIQ(source,dest,stats,scratch.data(),scratch.size())==65536);
 int pending=source.GetRingBufferReadAvailable();
 for(int i=0;i<1000;++i)assert(transferIQ(source,dest,stats,scratch.data(),scratch.size())==0);
 assert(source.GetRingBufferReadAvailable()==pending);assert(stats.dropped==0);
 // Slow consumer, burst producer: check every byte and the total, not just no crash.
 while(dest.GetRingBufferReadAvailable() || source.GetRingBufferReadAvailable()){
  int n=dest.getDataFromBuffer(scratch.data(),2048);output.insert(output.end(),scratch.begin(),scratch.begin()+n);
  transferIQ(source,dest,stats,scratch.data(),scratch.size());
 }
 assert(output==input);assert(stats.accepted==input.size());assert(stats.dropped==0);
 RingBuffer<uint8_t> a(8),z(8);IQWriteStats pair;
 a.putDataIntoBuffer(input.data(),8);z.putDataIntoBuffer(input.data(),7);
 assert(transferIQ(a,z,pair,scratch.data(),8)==0);assert(a.GetRingBufferReadAvailable()==8);
 z.FlushRingBuffer();assert(transferIQ(a,z,pair,scratch.data(),3)==2);
 assert(a.GetRingBufferReadAvailable()==6);
 std::cout<<"PASS: full destination leaves source intact; 3MiB burst/slow consumer byte-exact; IQ pair alignment; recovery; zero drops\n";
}
