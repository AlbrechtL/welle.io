#include "iq_buffer_pacer.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <algorithm>
struct Result {double queued,min,max,lost,elapsed;};
Result simulate(double ppm,bool adaptive,double hours,bool jitter=false){
 const double capacity=8388608,nominal=4096000,chunk=65536;
 IQBufferPacer p(nominal,capacity);double queue=capacity/2,t=0,lost=0,mn=queue,mx=queue;
 unsigned long n=0;
 while(t<hours*3600){
  const double amount=std::min(chunk,queue);queue-=amount;
  double dt=adaptive?p.secondsFor(amount,queue):amount/nominal;
  // Periodic extra wakeup delay is recovered on the next deadline. No clock jumps.
  if(jitter && n%1000==0)dt+=.1;
  if(jitter && n%1000>=1 && n%1000<=10)dt-=.01;
  assert(dt>0);
  queue+=nominal*(1+ppm/1e6)*dt;
  if(queue>capacity){lost+=queue-capacity;queue=capacity;}
  mn=std::min(mn,queue);mx=std::max(mx,queue);t+=dt;++n;
  // These cases should keep a reservoir; no synthetic starvation shortcut.
  assert(queue>chunk);
 }
 return {queue,mn,mx,lost,t};
}
int main(){
 auto old=simulate(47.569,false,8);assert(old.lost>1000000);
 std::cout<<"Old pacing +47.569ppm/8h loses "<<old.lost<<" bytes\n";
 for(double ppm:{-5000.,-500.,-47.569,0.,47.569,500.,5000.}){
  auto r=simulate(ppm,true,24);assert(r.lost==0);assert(r.min>3000000 && r.max<5500000);
  std::cout<<"24h "<<ppm<<"ppm: zero loss, final fill "<<r.queued<<"\n";
 }
 auto j=simulate(47.569,true,8,true);assert(j.lost==0);
 IQBufferPacer p(4096000,8388608);
 assert(p.bytesPerSecond(0)==4096000*.99);assert(p.bytesPerSecond(8388608)==4096000*1.01);
 assert(p.secondsFor(2,4194304)>0);
 std::cout<<"PASS: signed clock offsets, bounded correction, scheduler jitter, fractional small transfers\n";
}
