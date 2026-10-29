import socket,threading,time,subprocess,os,struct,re,pathlib,sys,tempfile
temporary=tempfile.TemporaryDirectory(prefix='welle-clock-')
b=pathlib.Path(temporary.name)
binary=pathlib.Path(sys.argv[1]).resolve()
s=socket.socket();s.bind(('127.0.0.1',0));s.listen();port=s.getsockname()[1]
stop=threading.Event()
def feed():
 c,_=s.accept();c.settimeout(2)
 try:
  c.sendall(struct.pack('!4sII',b'RTL0',5,29));block=b'\x80'*65536;next_tick=time.monotonic()
  while not stop.is_set():
   c.sendall(block);next_tick+=.016/1.005;stop.wait(max(0,next_tick-time.monotonic()))
 except OSError:pass
 finally:c.close()
t=threading.Thread(target=feed,daemon=True);t.start()
env=dict(os.environ)
with socket.socket() as h:h.bind(('127.0.0.1',0));http=h.getsockname()[1]
with (b/'test-tcp-clock-fast.log').open('w') as f:
 p=subprocess.Popen([str(binary),'-c','11D','-w',str(http),'-F','rtl_tcp,127.0.0.1:'+str(port)],stdout=f,stderr=f,env=env)
 try:
  time.sleep(40);assert p.poll() is None
 finally:
  p.terminate()
  try:rc=p.wait(timeout=10)
  except subprocess.TimeoutExpired:p.kill();p.wait();raise
  stop.set();s.close();t.join(timeout=3)
log=(b/'test-tcp-clock-fast.log').read_text();lines=[l for l in log.splitlines() if 'RTL_IQ_DIAG' in l]
assert rc==0,(rc,log[-2000:]);assert len(lines)>=5,log[-2000:]
for l in lines:
 d=dict((k,int(v)) for k,v in re.findall(r'(\w+)=(\d+)',l))
 # Counters are separate atomics: receive thread can update between reads.
 assert abs(d['net_offered']-d['net_accepted']-d['net_drop']) <= 65536
 assert d['net_drop']==0 and d['decoder_drop']==0
 assert d['decoder_offered']==d['decoder_accepted']+d['decoder_drop']
print('PASS: synthetic RTL-TCP input,',len(lines),'timestamped diagnostic lines, zero drop counters (network snapshots are not atomic), clean SIGTERM')
print(lines[-1])

last=dict((k,int(v)) for k,v in re.findall(r'(\w+)=(\d+)',lines[-1]))
assert last['net_drop']==0 and last['decoder_drop']==0
assert 4000000 < last['net_fill'] < 4750000,last
assert last['pace_bytes_s']>4105000,last
print('PASS: actual client follows +5000ppm TCP source; network reservoir bounded')
