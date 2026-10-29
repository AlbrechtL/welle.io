from pathlib import Path
import subprocess,sys
p=Path(sys.argv[2]);p.mkdir(parents=True,exist_ok=True)
source=Path(sys.argv[1])
s=(source/'src/backend/ofdm-decoder.cpp').read_text()
h=(source/'src/backend/ofdm-decoder.h').read_text()
def method(sig):
 a=s.index(sig);b=s.index('{',a);depth=0
 for i in range(b,len(s)):
  if s[i]=='{':depth+=1
  if s[i]=='}':
   depth-=1
   if depth==0:return s[a:i+1]
# Use the production state declarations and all lifecycle/handoff methods.
state=h[h.index('        // Set before'):h.index('        void workerthread(void);')]
header=r'''
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <complex>
#include <iostream>
#include <cassert>
#include <future>
#include <chrono>
using DSPCOMPLEX=std::complex<float>;
using namespace std::chrono_literals;
struct OfdmDecoder {
 struct {int L=76;int K=1536;} params;
 struct {void onConstellationPoints(std::vector<DSPCOMPLEX>&&) {}} radioInterface;
'''+state+r'''
 int constellationDecimation=96;
 std::vector<DSPCOMPLEX> constellationPoints;
 std::vector<int> seen;
 int slow_us=0;
 void processPRS(){seen.push_back(int(pending_symbols[0][0].real()));
   if(slow_us)std::this_thread::sleep_for(std::chrono::microseconds(slow_us));}
 void decodeDataSymbol(int){}
 void workerthread(); void stopWorker(); void reset(); ~OfdmDecoder();
 void pushAllSymbols(std::vector<std::vector<DSPCOMPLEX>>&&);
 void start(){thread=std::thread(&OfdmDecoder::workerthread,this);}
 void blockedProducer(){
   auto deadline=std::chrono::steady_clock::now()+5s;
   while(std::chrono::steady_clock::now()<deadline){
     {std::lock_guard<std::mutex> l(mutex);if(producer_waits>0)return;}
     std::this_thread::yield();
   }assert(false && "producer did not enter wait");
 }
 void drained(int n){
   auto deadline=std::chrono::steady_clock::now()+5s;
   while(std::chrono::steady_clock::now()<deadline){
     {std::lock_guard<std::mutex> l(mutex);if(processed_frames==unsigned(n))return;}
     std::this_thread::sleep_for(1ms);
   }assert(false && "drain timeout");
 }
};
std::vector<std::vector<DSPCOMPLEX>> frame(int id){return std::vector<std::vector<DSPCOMPLEX>>(76,std::vector<DSPCOMPLEX>(1,DSPCOMPLEX(id,0)));}
'''
methods='\n'.join(method(x) for x in ['OfdmDecoder::~OfdmDecoder()', 'void OfdmDecoder::stopWorker()', 'void OfdmDecoder::reset()', 'void OfdmDecoder::workerthread()', 'void OfdmDecoder::pushAllSymbols('])
main=r'''
int main(){
 // Notification before wait: first frame must survive delayed worker startup.
 {OfdmDecoder d;d.pushAllSymbols(frame(1));d.start();d.drained(1);d.stopWorker();assert((d.seen==std::vector<int>{1}));}
 // A second producer submission cannot overwrite a full, unread slot.
 {OfdmDecoder d;d.pushAllSymbols(frame(1));auto p=std::async(std::launch::async,[&]{d.pushAllSymbols(frame(2));});
 d.blockedProducer();assert(p.wait_for(20ms)==std::future_status::timeout);d.start();p.get();d.drained(2);d.stopWorker();assert((d.seen==std::vector<int>{1,2}));}
 // Empty/spurious wakeups do not decode stale memory.
 {OfdmDecoder d;d.start();for(int i=0;i<100;i++)d.pending_symbols_cv.notify_all();
 std::this_thread::sleep_for(10ms);d.stopWorker();assert(d.seen.empty());}
 // Slow consumer and producer bursts: retain every frame, once, in order.
 {OfdmDecoder d;d.slow_us=50;d.start();for(int i=0;i<1000;i++)d.pushAllSymbols(frame(i));
 d.drained(1000);d.stopWorker();assert(d.seen.size()==1000);for(int i=0;i<1000;i++)assert(d.seen[i]==i);}
 // Stop must release both an empty consumer and a producer blocked on a full slot.
 {OfdmDecoder d;d.start();d.stopWorker();}
 {OfdmDecoder d;d.pushAllSymbols(frame(1));auto p=std::async(std::launch::async,[&]{d.pushAllSymbols(frame(2));});
 d.blockedProducer();assert(p.wait_for(20ms)==std::future_status::timeout);d.stopWorker();assert(p.wait_for(1s)==std::future_status::ready);p.get();assert(d.submitted_frames==1);}
 // No worker resurrection when stop wins the startup race.
 for(int i=0;i<100;i++){OfdmDecoder d;d.start();d.stopWorker();assert(!d.running);}
 // Reset drops an old pending frame explicitly; an old blocked producer cannot
 // inject its stale frame into the next generation even if it wakes late.
 for(int i=0;i<50;i++){
   OfdmDecoder d;d.pushAllSymbols(frame(1));auto p=std::async(std::launch::async,[&]{d.pushAllSymbols(frame(2));});
   d.blockedProducer();assert(p.wait_for(5ms)==std::future_status::timeout);d.reset();p.get();d.pushAllSymbols(frame(3));
   d.drained(1);d.stopWorker();assert((d.seen==std::vector<int>{3}));assert(d.cancelled_frames==1);
 }
 // Repeated real reset/start/stop paths with consumed frames.
 {OfdmDecoder d;d.start();for(int i=0;i<50;i++){d.pushAllSymbols(frame(i));d.drained(i+1);d.reset();}d.stopWorker();assert(d.seen.size()==50);}
 std::cout<<"PASS: 9 scenarios; delayed notification, no overwrite, spurious wakeups, 1000 ordered frames, stop/unblock, startup race, reset generation, repeated reset\n";
}
'''
(p/'test-frame.cpp').write_text(header+methods+main)
subprocess.run(['g++','-std=c++14','-pthread','-O2','-Wall','-Wextra','-Werror',str(p/'test-frame.cpp'),'-o',str(p/'test-frame')],check=True)
r=subprocess.run([str(p/'test-frame')],capture_output=True,text=True,timeout=30)
(p/'test-frame.log').write_text(r.stdout+r.stderr)
print(r.stdout);print('exit',r.returncode);r.check_returncode()
