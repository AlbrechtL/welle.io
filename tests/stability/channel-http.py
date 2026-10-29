#!/usr/bin/env python3
"""Check the real CLI's -c option and generated HTML with synthetic I/Q."""
import pathlib,subprocess,tempfile,socket,urllib.request,time,signal,sys
binary=pathlib.Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory(prefix='welle-channel-ui-') as tmp:
 tmp=pathlib.Path(tmp);iq=tmp/'quiet.u8.iq'
 with iq.open('wb') as f:
  for _ in range(32):f.write(bytes([128])*1024*1024)
 for args,selectable in [([],True),(['-c','5C'],False),(['-c','10B'],False)]:
  with socket.socket() as s:s.bind(('127.0.0.1',0));port=s.getsockname()[1]
  with (tmp/'run.log').open('w') as log:
   p=subprocess.Popen([str(binary),'-f',str(iq),'-w',str(port),'-T']+args,stdout=log,stderr=log)
   try:
    for _ in range(60):
     if p.poll() is not None:raise RuntimeError('CLI exited before HTTP startup')
     try:
      with urllib.request.urlopen('http://127.0.0.1:'+str(port)+'/',timeout=2) as r:html=r.read().decode()
      break
     except OSError:time.sleep(.1)
    else:raise RuntimeError('HTTP startup timeout')
    assert ('id="channelselector"' in html)==selectable,(args,html)
    assert ('id="channelvalue"' in html)==(not selectable)
    assert '<!-- channel-control -->' not in html
    with urllib.request.urlopen('http://127.0.0.1:'+str(port)+'/index.js?cachebreak=1',timeout=2) as r:js=r.read().decode()
    assert 'if (selector)' in js and 'serviceIdForUrl' in js
    print('PASS:',args or 'no -c','dropdown',selectable,flush=True)
   finally:
    if p.poll() is None:p.send_signal(signal.SIGTERM)
    try:code=p.wait(timeout=8)
    except subprocess.TimeoutExpired:p.kill();p.wait();raise
    assert code==0,code
