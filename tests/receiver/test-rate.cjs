const fs=require('fs'), vm=require('vm'), assert=require('assert');
const code=fs.readFileSync(__dirname+'/../../src/welle-cli/index.js','utf8');
const context={};vm.createContext(context);
vm.runInContext(code.slice(code.indexOf('function createIQRateMeter()'),code.indexOf('// Bounded browser-only history.')),context);
const sample=(t,bytes=t*4096000,g=1)=>({bytes_received:bytes,generation:g,monotonic_seconds:t,seconds_since_data:0.01,bytes_per_sample:2});
let meter=context.createIQRateMeter(),r;
assert.equal(meter(null).state,'unavailable');
for(let t=0;t<=120;t++)r=meter(sample(t));
assert.equal(r.state,'ready');assert.match(context.iqRateHTML(r),/RTL-TCP/);assert.equal(r.seconds,60);assert.equal(r.mbits,32.768);assert.equal(r.msamples,2.048);
assert.match(context.iqRateHTML(r),/32.7680 Mbit.s/);assert.match(context.iqRateHTML(r),/2.04800 MS.s/);
// Actual input offsets and alternating bursts; never substitute nominal rate.
for(const rate of [4096200,4075520,4116480]){
 meter=context.createIQRateMeter();
 for(let t=0;t<=70;t++)r=meter(sample(t,t*rate+(t%2)*65536));
 assert.equal(r.mbits,rate*8/1e6);assert.equal(r.msamples,rate/2e6);
}
assert.equal(meter(sample(71,1,2)).state,'warming'); // reconnect
assert.equal(meter(sample(72,0,2)).state,'warming'); // counter reset
assert.equal(meter(sample(80,1e7,2)).state,'warming'); // background tab
assert.equal(meter(sample(79,1e7,2)).state,'warming'); // old reply
assert.equal(meter({...sample(81),seconds_since_data:4}).state,'stale');
assert.equal(meter(sample(82)).state,'warming');
assert.equal(meter({...sample(83),bytes_received:NaN}).state,'unavailable');
assert.equal(meter({...sample(84),bytes_per_sample:0}).state,'unavailable');
assert.equal(meter({...sample(85),bytes_received:Number.MAX_SAFE_INTEGER+1}).state,'unavailable');
console.log('PASS: measured rate, 60s bound, signed offsets, bursts, warmup, reconnect/reset, pause, stale, invalid data');

meter=context.createIQRateMeter();
for(let t=0;t<65;t++)r=meter({...sample(t,t*2048000*8),bytes_per_sample:8});
assert.equal(r.msamples,2.048);assert.equal(r.mbits,131.072);
assert(context.iqRateHTML(r).includes('Soapy CF32'));assert(context.iqRateHTML(r).includes('keine USB-Bitrate'));
console.log('PASS: Soapy CF32 eight bytes per pair; 2.048 MS/s, 131.072 internal Mbit/s, explicit USB distinction');
