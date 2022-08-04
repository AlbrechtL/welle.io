#!/usr/bin/python3

import json

server_ip_address = '10.0.2.217';
server_port_nr    = '8888';

def gen_m3u ( station ):
    name = station [ 'stationName' ] . rstrip () 
    sid  = station [ 'stationSId'  ]
    chn  = station [ 'channelName' ]
    print  ( name , hex(sid) , chn )
    with open ( 'DAB '+name+'.m3u' , 'w' ) as p:
        p.write ( '#EXTM3U\n#EXTINF:-1,1 '+name+'\nhttp://'+server_ip_address+':'+server_port_nr+'/mp3/'+hex(sid)+'/'+chn+'\n' )
        p.close 

with open('stations.json') as f:
    stations = json.load(f)
    for station in stations:
        gen_m3u ( station )
