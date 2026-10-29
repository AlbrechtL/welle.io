// Link the actual HTTP router, MOT cache and Socket implementation.
// Expose only test access to the cache/dispatcher; no SDR is opened.
#include <bits/stdc++.h>
#define private public
#include "welle-cli/webradiointerface.h"
#undef private
#include "input/null_device.h"
#include "backend/radio-receiver.h"
#include <cassert>
using namespace std;
static string request(WebRadioInterface& web,const string& path) {
 int fds[2];assert(socketpair(AF_UNIX,SOCK_STREAM,0,fds)==0);
 Socket server;server.sock=fds[0];assert(server.setNonBlocking());
 string req="GET "+path+" HTTP/1.0\r\n\r\n";
 assert(::send(fds[1],req.data(),req.size(),MSG_NOSIGNAL)==(ssize_t)req.size());
 shutdown(fds[1],SHUT_WR);web.dispatch_client(move(server));
 string response;char buf[4096];ssize_t n;
 while((n=::recv(fds[1],buf,sizeof(buf),0))>0)response.append(buf,n);
 ::close(fds[1]);return response;
}
int main() {
 CNullDevice input;WebRadioInterface::DecodeSettings settings{};settings.outputCodec=OutputCodec::MP3;
 WebRadioInterface web(input,0,settings,RadioReceiverOptions{});
 web.running=false;web.programme_handler_thread.join();
 auto handler=make_shared<WebProgrammeHandler>(0x15dc,OutputCodec::MP3);
 mot_file_t mot{};mot.content_sub_type=3;
 mot.data={137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,1,0,0,0,1,8,4,0,0,0,181,28,12,2,0,0,0,11,73,68,65,84,120,218,99,100,248,15,0,1,5,1,1,39,24,227,102,0,0,0,0,73,69,78,68,174,66,96,130};
 handler->onMOT(mot);
 {lock_guard<mutex> lock(web.rx_mut);web.phs[0x15dc]=handler;}
 for(const auto& path:{"/slide/0x15dc","/slide/0x15dc?cachebreak=1790974077","/slide/0x15dc?cachebreak=2&other=3"}) {
  const auto response=request(web,path);
  cout<<path<<" -> "<<response.substr(0,response.find("\r\n"))<<endl;
  assert(response.find("HTTP/1.0 200 OK\r\n")==0);
  assert(response.find("Content-Type: image/png\r\n")!=string::npos);
  assert(response.substr(response.find("\r\n\r\n")+4)==string(mot.data.begin(),mot.data.end()));
 }
 assert(request(web,"/slide/not-a-sid?cachebreak=1").find("HTTP/1.0 400 ")==0);
 assert(request(web,"/slide/0xffff?cachebreak=1").find("HTTP/1.0 404 ")==0);
 assert(request(web,"/index.js?cachebreak=1").find("HTTP/1.0 200 ")==0);
 cout<<"PASS: actual SLS router with cached PNG, query parameters, payload bytes, invalid and absent IDs."<<endl;
}
