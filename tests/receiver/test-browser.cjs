const {chromium}=require('playwright');
const net=require('net'),fs=require('fs'),cp=require('child_process'),assert=require('assert');
const b=fs.mkdtempSync(require('path').join(require('os').tmpdir(),'welle-browser-'));
console.log('Test artifacts:',b);
const binary=process.argv[2]; if(!binary) throw Error('Usage: node test-browser.cjs /absolute/path/welle-cli');
const sleep=ms=>new Promise(r=>setTimeout(r,ms));
let client,timer,child,browser,server,sent=0;
const block=Buffer.alloc(65536,128);
function startFeed(){timer=setInterval(()=>{if(client&&!client.destroyed){client.write(block);sent+=block.length;}},16);}
async function json(port){const r=await fetch('http://127.0.0.1:'+port+'/mux.json');return r.json();}
async function until(fn,timeout=15000){const end=Date.now()+timeout;let error;while(Date.now()<end){try{const v=await fn();if(v)return v;}catch(e){error=e;}await sleep(100);}throw error||Error('timeout');}
(async()=>{
 server=net.createServer(c=>{client=c;c.on('data',()=>{});c.on('error',()=>{});
   const header=Buffer.alloc(12);header.write('RTL0');header.writeUInt32BE(5,4);header.writeUInt32BE(29,8);
   c.write(header.subarray(0,4));setTimeout(()=>{if(!c.destroyed){c.write(header.subarray(4));c.write(Buffer.alloc(12345,128));sent=12345;}},40);
 });
 await new Promise(r=>server.listen(0,'127.0.0.1',r));
 const reserve=net.createServer();await new Promise(r=>reserve.listen(0,'127.0.0.1',r));const httpPort=reserve.address().port;await new Promise(r=>reserve.close(r));
 const out=fs.openSync(b+'/test-iq-scale-browser.log','w');
 child=cp.spawn(binary,['-c','11D','-w',String(httpPort),'-F','rtl_tcp,127.0.0.1:'+server.address().port],{env:process.env,stdio:['ignore',out,out]});
 fs.closeSync(out);
 const initial=await until(async()=>{const j=await json(httpPort);return j.receiver.hardware.iq_stream?.bytes_received===12345&&j;});
 assert.equal(initial.receiver.hardware.iq_stream.bytes_per_sample,2);
 console.log('PASS: real TCP split 12-byte header excluded; partial 12345-byte payload counted exactly');
 browser=await chromium.launch({headless:true,args:['--no-sandbox']});const page=await browser.newPage();const errors=[];page.on('pageerror',e=>errors.push(e.message));
 await page.goto('http://127.0.0.1:'+httpPort+'/');
 assert.equal(await page.locator('#iq-rate-plot').isVisible(),false);
 await page.locator('#iq-rate-history .title').click();
 assert.equal(await page.locator('#iq-rate-history .title').getAttribute('aria-expanded'),'true');
 startFeed();
 await page.waitForFunction(()=>document.querySelector('#iq-rate')?.textContent.includes('Mbit/s'),{},{timeout:20000});
 const value=await page.locator('#iq-rate').innerText();const n=parseFloat(value);assert(n>28&&n<36,value);
 assert(value.includes('MS/s'));assert.equal(await page.locator('#channelvalue').innerText(),'11D');
 await page.screenshot({path:b+'/iq-scale-preview.png',fullPage:false});
 console.log('PASS: native browser status displays actual IQ RX:',value.replace(/\n/g,' / '));
 const graph=await page.evaluate(()=>iqRateHistory.view(performance.now()/1000));
 assert.equal(graph.minimum,2025);assert.equal(graph.maximum,2075);assert.equal(graph.starting,true);
 assert.equal(graph.centre,2048);assert(graph.points.some(p=>p.value!==null&&p.mean!==null));
 const countBefore=graph.points.length;
 await page.locator('#iq-rate-history .title').click();
 assert.equal(await page.locator('#iq-rate-plot').isVisible(),false);
 await page.waitForTimeout(2200);
 assert(await page.evaluate(n=>iqRateHistory.view(performance.now()/1000).points.length>n,countBefore));
 await page.locator('#iq-rate-history .title').press('Enter');
 assert.equal(await page.locator('#iq-rate-plot').isVisible(),true);
 await page.locator('#iq-rate-plot').click({position:{x:110,y:80}});
 assert.equal(await page.locator('#iq-rate-plot').isVisible(),true);
 console.log('PASS: default folded, click/keyboard toggle, collection continues folded, plot click stays open');
 await page.waitForFunction(()=>document.querySelector('#iq-rate-plot-summary').textContent.includes('kS/s'));
 const axisLabels=await page.evaluate(()=>{
   const labels=[],original=CanvasRenderingContext2D.prototype.fillText;
   CanvasRenderingContext2D.prototype.fillText=function(text,x,y){labels.push(text);return original.apply(this,arguments);};
   drawIQRateHistory();CanvasRenderingContext2D.prototype.fillText=original;return labels;
 });
 assert(axisLabels.includes('2025')&&axisLabels.includes('2075')&&axisLabels.includes('2048'));
 await page.locator('#iq-rate-history').screenshot({path:b+'/iq-scale-startup-preview.png'});
 console.log('PASS: canvas startup limits2025/2075 and nominal2048 rendered');
 const label=await page.evaluate(()=>{
   const calls=[],original=CanvasRenderingContext2D.prototype.fillText;
   CanvasRenderingContext2D.prototype.fillText=function(text,x,y){calls.push({text,x,y,colour:this.fillStyle,align:this.textAlign});return original.apply(this,arguments);};
   drawIQRateHistory();CanvasRenderingContext2D.prototype.fillText=original;
   return calls.find(c=>c.text.startsWith('Ø '));
 });
 assert(label&&/^Ø [0-9]+\.[0-9]{3} kS\/s$/.test(label.text),JSON.stringify(label));
 assert.equal(label.align,'right');assert.equal(label.y,12);assert.equal(label.colour,'#7b3fa2');
 console.log('PASS: numeric current mean drawn top right in purple:',label.text);
 // A controlled visual fixture checks small deviations on both sides of 2048.
 await page.evaluate(()=>{iqRateHistory=createIQRateHistory();const now=performance.now()/1000;for(let i=0;i<=120;i++)iqRateHistory.add(now-120+i,{state:'ready',msamples:2.048+0.0003*Math.sin(i/12)});latestIQPlotRate={state:'ready',msamples:2.048+0.0003*Math.sin(10),seconds:60};drawIQRateHistory();});
 await page.locator('#iq-rate-history').screenshot({path:b+'/iq-scale-centre-preview.png'});
 await page.setViewportSize({width:390,height:900});
 await page.locator('#iq-rate-history').screenshot({path:b+'/iq-scale-mobile-preview.png'});
 assert((await page.locator('#iq-rate-plot').boundingBox()).width<=390);
 console.log('PASS: graph contains actual points, centre2048, responsive desktop/mobile canvas');
 clearInterval(timer);timer=null;
 await page.waitForFunction(()=>document.querySelector('#iq-rate')?.textContent==='Kein IQ-Empfang',{},{timeout:10000});
 const stalled=(await json(httpPort)).receiver.hardware.iq_stream;assert.equal(stalled.bytes_received,sent);
 await page.waitForFunction(()=>document.querySelector('#iq-rate-plot-summary').textContent.includes('Kein IQ-Empfang'));
 assert(await page.evaluate(()=>iqRateHistory.view(performance.now()/1000).points.some(p=>p.value===null)));
 console.log('PASS: IQ stop reports no reception; graph gap; all transmitted payload counted, without header');
 startFeed();await page.waitForFunction(()=>document.querySelector('#iq-rate')?.textContent.includes('Mbit/s'),{},{timeout:15000});
 await page.route('**/mux.json',r=>r.abort());
 await page.waitForFunction(()=>document.querySelector('#iq-rate')?.textContent==='Status veraltet',{},{timeout:12000});
 await page.waitForFunction(()=>document.querySelector('#iq-rate-plot-summary').textContent.includes('Status veraltet'));
 assert.deepEqual(errors,[]);console.log('PASS: recovery, HTTP outage graph gap, zero browser exceptions');
})().catch(e=>{console.error(e);process.exitCode=1;}).finally(async()=>{
 clearInterval(timer);if(browser)await browser.close();
 if(child){const ended=new Promise(r=>child.once('exit',(code,signal)=>r({code,signal})));child.kill('SIGTERM');const result=await Promise.race([ended,sleep(10000).then(()=>null)]);if(!result){child.kill('SIGKILL');await ended;process.exitCode=1;}else if(result.code!==0){console.error('Bad child exit',result);process.exitCode=1;}}
 if(client)client.destroy();if(server)server.close();
});
