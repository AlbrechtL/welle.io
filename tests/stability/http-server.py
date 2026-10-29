#!/usr/bin/env python3
"""Run the real CLI with synthetic quiet I/Q; never opens a radio device."""
from pathlib import Path
import tempfile, subprocess, socket, time, signal, json, sys, concurrent.futures
binary=Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory(prefix='welle-http-test-') as name:
    temp=Path(name); iq=temp/'quiet.u8.iq'
    with iq.open('wb') as f:
        for _ in range(32): f.write(bytes([128])*1024*1024)
    with socket.socket() as reserve:
        reserve.bind(('127.0.0.1',0));port=reserve.getsockname()[1]
    with (temp/'server.log').open('w') as log:
        process=subprocess.Popen([str(binary),'-f',str(iq),'-c','5C','-w',str(port),'-T'],stdout=log,stderr=log)
        def request(data):
            with socket.create_connection(('127.0.0.1',port),timeout=3) as sock:
                sock.sendall(data); result=b''
                while True:
                    chunk=sock.recv(65536)
                    if not chunk: return result
                    result+=chunk
        def resources():
            return {'threads':len(list(Path(f'/proc/{process.pid}/task').iterdir())), 'fds':len(list(Path(f'/proc/{process.pid}/fd').iterdir()))}
        clients=[]
        try:
            for _ in range(100):
                if process.poll() is not None: raise RuntimeError('server exited at startup')
                try:
                    response=request(b'GET /mux.json HTTP/1.0\r\n\r\n')
                    assert b'200 OK' in response
                    break
                except (OSError,AssertionError):time.sleep(.1)
            else: raise RuntimeError('no healthy server')
            time.sleep(.3);baseline=resources()
            def cycle(n):
                with socket.create_connection(('127.0.0.1',port),timeout=3) as sock:
                    if n%3==0:sock.sendall(b'GET /mux.json HTTP/1.0\r\n')
                    elif n%3==1:sock.sendall(b'POST /channel HTTP/1.0\r\nContent-Length: -1\r\n\r\n')
                    else:sock.sendall(b'GET /mp3/not-a-sid HTTP/1.0\r\n\r\n')
            with concurrent.futures.ThreadPoolExecutor(max_workers=8) as pool:list(pool.map(cycle,range(240)))
            time.sleep(.4)
            assert b'200 OK' in request(b'GET /mux.json HTTP/1.0\r\n\r\n')
            for _ in range(80):
                try:clients.append(socket.create_connection(('127.0.0.1',port),timeout=2))
                except OSError:pass
            peak=resources();assert peak['threads']<=baseline['threads']+64
            time.sleep(6)
            for client in clients:client.close()
            clients=[]
            assert b'200 OK' in request(b'GET /mux.json HTTP/1.0\r\n\r\n')
            time.sleep(.4);after=resources()
            assert after['threads']<=baseline['threads']+2,(baseline,after)
            assert after['fds']<=baseline['fds']+2,(baseline,after)
            fic=socket.create_connection(('127.0.0.1',port),timeout=3);clients.append(fic)
            fic.sendall(b'GET /fic HTTP/1.0\r\n\r\n');assert b'200 OK' in fic.recv(4096)
            # Keep a partial HTTP request and a silent FIC stream open at shutdown.
            clients.append(socket.create_connection(('127.0.0.1',port),timeout=3))
            clients[-1].sendall(b'GET /mux.json HTTP/1.0\r\n')
            start=time.monotonic();process.send_signal(signal.SIGTERM);code=process.wait(timeout=8)
            assert code==0,code
            print(json.dumps({'real_server':'OK','aborted_requests':240,'baseline':baseline,'peak':peak,'after':after,'shutdown_seconds':round(time.monotonic()-start,3)},indent=2))
        finally:
            for client in clients:client.close()
            if process.poll() is None:process.kill();process.wait()
            if process.returncode!=0:print((temp/'server.log').read_text()[-8000:])
