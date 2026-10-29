#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <future>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <vector>
#include <cctype>
#include <cerrno>
#include <cstring>
#include "various/Socket.h"
#include "welle-cli/webprogrammehandler.h"
#include "backend/mot_manager.h"
#include "various/ringbuffer.h"
using namespace std;
#include "http-parser.inc"

struct Pair {
    Socket client;
    int peer;
    Pair() {
        int listener = socket(AF_INET, SOCK_STREAM, 0); assert(listener >= 0);
        sockaddr_in addr{}; addr.sin_family=AF_INET; addr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
        assert(::bind(listener, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0);
        assert(::listen(listener, 1) == 0);
        socklen_t len=sizeof(addr); assert(getsockname(listener,reinterpret_cast<sockaddr*>(&addr),&len)==0);
        assert(client.connect("127.0.0.1",ntohs(addr.sin_port),1));
        peer=::accept(listener,nullptr,nullptr); assert(peer>=0); ::close(listener);
        assert(client.setNonBlocking());
        timeval tv{3,0}; setsockopt(peer,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));
    }
    ~Pair(){if(peer>=0)::close(peer);}
    void closePeer(){::close(peer);peer=-1;}
    void expect(const vector<uint8_t>& bytes) {
        vector<uint8_t> actual(bytes.size()); size_t n=0;
        while(n<bytes.size()){auto k=::recv(peer,actual.data()+n,actual.size()-n,0);assert(k>0);n+=k;}
        assert(actual==bytes);
    }
};
static void http(const string& request, bool valid, const string& body="") {
    Pair p;
    assert(::send(p.peer,request.data(),request.size(),MSG_NOSIGNAL)==(ssize_t)request.size());
    shutdown(p.peer,SHUT_WR);
    auto result=parse_http_headers(p.client);
    assert(result.valid==valid);
    if(valid)assert(result.post_data==body);
}
static void httpTests(){
    {
        Pair p;
        const string request="GET /slide/0x15dc?cachebreak=1790974077 HTTP/1.1\r\n\r\n";
        assert(::send(p.peer,request.data(),request.size(),MSG_NOSIGNAL)==(ssize_t)request.size());
        shutdown(p.peer,SHUT_WR);
        auto result=parse_http_headers(p.client);
        assert(result.valid && result.url=="/slide/0x15dc");
    }
    http("GET /mux.json HTTP/1.1\r\nHost: localhost\r\n\r\n",true);
    http("GET /mux.json HTTP/1.1\r\n",false);
    http("POST /channel HTTP/1.1\r\nContent-Length: -1\r\n\r\n",false);
    http("POST /channel HTTP/1.1\r\ncontent-length: 2\r\n\r\n12",true,"12");
    http("POST /channel HTTP/1.1\r\nContent-Length: 0\r\n\r\n",true);
    http("POST /channel HTTP/1.1\r\nContent-Length: 3\r\n\r\n12",false);
    http("POST /channel HTTP/1.1\r\nContent-Length: 2x\r\n\r\n12",false);
    http("POST /channel HTTP/1.1\r\nContent-Length: 999999999999999999999\r\n\r\n",false);
    http("POST /channel HTTP/1.1\r\nContent-Length: 1\r\ncontent-length: 1\r\n\r\na",false);
    http("POST /channel HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n",false);
    http("GET / HTTP/1.1\r\nReferer: http://localhost:8000/\r\n\r\n",true);
    http("GET / HTTP/1.1\r\nX: "+string(8200,'a')+"\r\n\r\n",false);
    Pair idle; auto start=chrono::steady_clock::now();
    assert(!parse_http_headers(idle.client).valid);
    assert(chrono::steady_clock::now()-start<chrono::seconds(7));
    for(auto value:{"c123","C123","0xc123","49443","0d49443"}){uint32_t sid=0;assert(parse_service_id(value,sid));assert(sid==0xc123);}
    for(auto value:{"","-1","123z","999999999999","0x","1/2"}){uint32_t sid=0;assert(!parse_service_id(value,sid));}
    {uint32_t sid=0;assert(parse_service_id("0d12",sid));assert(sid==0x0d12);}
    cout<<"HTTP/SID: OK"<<endl;
}
static void audioTests(){
    { Pair p; ProgrammeSender sender(move(p.client));
      auto worker=async(launch::async,[&]{sender.wait_for_termination();});
      assert(sender.send_stream({1,2},{3,4}));p.expect({1,2,3,4});
      assert(sender.send_stream({1,2},{5,6}));p.expect({5,6});
      sender.cancel();assert(worker.wait_for(chrono::seconds(2))==future_status::ready);worker.get(); }
    { Pair p; ProgrammeSender sender(move(p.client)); p.closePeer();
      auto worker=async(launch::async,[&]{sender.wait_for_termination();});
      assert(worker.wait_for(chrono::seconds(2))==future_status::ready);worker.get(); }
    { Pair p; ProgrammeSender sender(move(p.client));
      assert(!sender.send_stream({},vector<uint8_t>(1024*1024+1))); }
    { Pair p; ProgrammeSender sender(move(p.client));
      auto worker=async(launch::async,[&]{sender.wait_for_termination();});
      vector<uint8_t> chunk(16384,1); bool rejected=false;
      auto start=chrono::steady_clock::now();
      for(int n=0;n<4096;n++)if(!sender.send_stream({},chunk)){rejected=true;break;}
      assert(rejected);assert(chrono::steady_clock::now()-start<chrono::seconds(2));
      assert(worker.wait_for(chrono::seconds(3))==future_status::ready);worker.get(); }
    { Pair p; vector<uint8_t> bytes(8*1024*1024,7);
      auto start=chrono::steady_clock::now();
      assert(p.client.sendAll(bytes.data(),bytes.size(),MSG_NOSIGNAL,100)<0);
      assert(chrono::steady_clock::now()-start<chrono::seconds(2)); }
#ifdef HAVE_FLAC
    { Pair p; ProgrammeSender sender(move(p.client));
      WebProgrammeHandler handler(1,OutputCodec::FLAC);
      handler.registerSender(&sender);
      auto worker=async(launch::async,[&]{sender.wait_for_termination();});
      handler.onNewAudio(vector<int16_t>(16384,0),48000,"stereo");
      p.expect({'f','L','a','C'});
      sender.cancel();worker.get();handler.removeSender(&sender); }
    cout<<"FLAC encoding/header/teardown: OK"<<endl;
#endif
    cout<<"Audio queue/idle disconnect/backpressure: OK"<<endl;
}
static void motTests(){
    for(bool category:{false,true}){
      vector<uint8_t> data(category?9:8,0);data[4]=data.size()>>1;data[5]=(data.size()&1)<<7;data[7]=category?0x65:0x04;
      MOTObject object;object.AddSeg(true,0,true,data.data(),data.size());assert(!object.IsToBeShown());
    }
    cout<<"MOT bounds: OK"<<endl;
}
static void ringTests(){
    RingBuffer<uint32_t> ring(1024); constexpr uint32_t count=100000;
    thread producer([&]{for(uint32_t n=0;n<count;){if(ring.GetRingBufferWriteAvailable()>0)n+=ring.putDataIntoBuffer(&n,1);}});
    for(uint32_t n=0;n<count;){uint32_t value;if(ring.getDataFromBuffer(&value,1)){assert(value==n);++n;}}
    producer.join();assert(ring.GetRingBufferReadAvailable()==0);
    cout<<"Ring buffer producer/consumer: OK"<<endl;
}
int main(){httpTests();audioTests();motTests();ringTests();}
