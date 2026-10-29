// Exercise the real page startup for both rendered channel-control variants.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const path = require('node:path');
const source = fs.readFileSync(path.join(__dirname, '../../src/welle-cli/index.js'), 'utf8');
for (const selectable of [false, true]) {
 const elements = new Map(), requests = [];
 function element(id) {
  if ((id === 'channelselector' && !selectable) || (id === 'channelvalue' && selectable)) return null;
  if (!elements.has(id)) elements.set(id, {style: {}, options: {items: [], add(o) {this.items.push(o);}}, getElementsByClassName(name) {return [element(id+'/'+name)];}});
  return elements.get(id);
 }
 function XHR() {
  this.open=(method,url)=>{this.method=method;this.url=url;};
  this.setRequestHeader=()=>{};
  this.send=body=>{
   requests.push({method:this.method,url:this.url,body});
   if (this.method==='GET' && this.url==='/channel') {
    this.readyState=4;this.status=200;this.responseText='5C';this.onreadystatechange();
   }
  };
 }
 const c=vm.createContext({window:{addEventListener(){}},document:{getElementById:element,getElementsByClassName:id=>[element(id)],createElement:()=>({})},XMLHttpRequest:XHR,setInterval:()=>1});
 vm.runInContext(source,c);c.window.onload();
 assert.equal(requests.filter(r=>r.method==='POST').length,0);
 if (selectable) {
  assert.equal(element('channelselector').value,'5C');
  assert.equal(element('channelselector').options.items.length,c.channels.length);
  element('channelselector').value='5D';element('channelselector').onchange();
  assert.deepEqual(requests.at(-1),{method:'POST',url:'/channel',body:'5D'});
 } else assert.equal(element('channelvalue').textContent,'5C');
}
console.log('PASS: actual page startup, fixed channel text and optional dropdown/change action.');
