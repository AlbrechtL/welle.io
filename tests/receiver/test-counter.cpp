#include "src/input/iq_stream_counter.h"
#include <cassert>
#include <thread>
#include <atomic>
int main() {
    IQStreamCounter c;
    assert(!c.snapshot().available);
    c.begin(); auto s=c.snapshot(); assert(s.generation==1 && !s.available);
    c.record(0); assert(!c.snapshot().available);
    c.record(17); s=c.snapshot(); assert(s.available && s.bytes==136 && s.bytesPerSample==8 && s.secondsSinceData>=0);
    c.begin();s=c.snapshot();assert(s.generation==2 && s.bytes==0 && !s.available);
    std::atomic<bool> done{false};
    std::thread producer([&]{for(int i=0;i<100000;i++)c.record(31);done=true;});
    uint64_t last=0;
    while(!done){s=c.snapshot();assert(s.bytes>=last && s.bytes%248==0 && s.generation==2 && s.secondsSinceData>=0);last=s.bytes;}
    producer.join();assert(c.snapshot().bytes==24800000);
}
