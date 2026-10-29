const fs=require('fs'),vm=require('vm'),assert=require('assert');
const code=fs.readFileSync(__dirname+'/../../src/welle-cli/index.js','utf8');
const context={};vm.createContext(context);
vm.runInContext(code.slice(code.indexOf('function createIQRateHistory()'),code.indexOf('var iqRateHistory =')),context);
let h=context.createIQRateHistory(),v=h.view(100);
assert.equal(v.centre,2048);assert.equal(v.span,1);assert.equal(v.points.length,0);
h.add(101,{state:'ready',msamples:2.0481});h.add(102,{state:'ready',msamples:2.0479});
v=h.view(102);assert.equal(v.centre,2048);assert.equal(v.points[0].value,2048.1);assert(Math.abs(v.points[1].value-2047.9)<1e-9);
h.add(103,{state:'stale'});h.add(104,{state:'warming'});h.add(105,{state:'ready',msamples:2.06});
v=h.view(105);assert.equal(v.points[2].value,null);assert.equal(v.points[3].value,null);assert(v.span>12);assert.equal(v.centre,2048);
h.add(105,{state:'ready',msamples:2});assert.equal(h.view(105).points.length,5); // no duplicate/out-of-order points
for(let i=106;i<2000;i++)h.add(i,{state:'ready',msamples:2.048});
v=h.view(1999);assert(v.points.length<=901);assert(v.points[0].t>=1099);assert.equal(v.end-v.start,900);
assert.equal(h.view(3000).points.length,0); // stale history expires
h=context.createIQRateHistory();for(let i=0;i<2000;i++)h.add(i/100,{state:'ready',msamples:2.048});assert(h.view(20).points.length<=1000);
console.log('PASS: fixed 2048 centre, symmetric scale, measured points, gaps, no duplicate time, 15min expiry and hard memory bound');
