
int main() {
  assert(std::abs(agcNextGain(8,0,10.2,.1,false)-7.9)<1e-9);
  assert(std::abs(agcNextGain(8,0,10.2,.1,true)-8.1)<1e-9);
  assert(std::abs(agcNextGain(double(float(8.1)),0,10.2,.1,false)-8)<1e-9);
  assert(agcNextGain(0,0,10.2,.1,false)==0);
  assert(agcNextGain(10.2,0,10.2,.1,true)==10.2);
  assert(agcNextGain(80,0,10.2,.1,false)==10.2);
  assert(agcNextGain(8,0,10,0,true)==9);
  assert(agcNextGain(8,10,0,.1,true)==8);
  unsigned low=0; AgcWindow a;
  for(int i=0;i<10000;++i)a.add(.1,0,1);
  assert(a.direction(low)==0); assert(a.direction(low)==1);
  AgcWindow good; for(int i=0;i<10000;++i)good.add(5.6,0,8);
  assert(good.direction(low)==0);
  good.add(8,0,8); assert(good.direction(low)==0); // one rare spike
  AgcWindow clipped; for(int i=0;i<10000;++i)clipped.add(8,0,8);
  assert(clipped.direction(low)==-1);
  AgcWindow zero; for(int i=0;i<10000;++i)zero.add(0,0,8);
  assert(zero.direction(low)==0);
  AgcWindow invalid=a; invalid.add(NAN,0,1); assert(invalid.direction(low)==0);
  AgcWindow normalized, legacy;
  for(int i=0;i<10000;++i){double v=(i%1000)/1000.0*.7; normalized.add(v,v*.2,1);legacy.add(v*8,v*1.6,8);}
  assert(normalized.histogram==legacy.histogram);
  // Settled level at native80 must be held, native90 clipping reduced.
  unsigned l=0; assert(good.direction(l)==0); assert(clipped.direction(l)==-1);
  std::cout<<"PASS: gain steps/bounds/rounding; scaled IQ; hysteresis; spike rejection; clipping; zero/invalid windows\n";
}
