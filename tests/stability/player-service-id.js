// Run with node tests/stability/player-service-id.js [path/to/index.js].
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const elements = new Map();
function element(id) {
    if (!elements.has(id)) elements.set(id, {style: {}, load() {}, play() {}});
    return elements.get(id);
}
const context = vm.createContext({
    window: {},
    document: {getElementById: element, getElementsByClassName: id => [element(id)]},
    setInterval() { return 1; },
});
const source = process.argv[2] || path.join(__dirname, '../../src/welle-cli/index.js');
vm.runInContext(fs.readFileSync(source, 'utf8'), context);
// Exercise the real rendered onclick: hexadecimal literals become JS numbers.
for (const sid of ['0x15dc', '0x100d', '0x1000', '0xd210']) {
    const html = context.parseTemplate(context.serviceTemplate(), {SId: sid});
    const click = html.match(/onclick="([^"]+)"/)[1];
    vm.runInContext(click, context);
    assert.equal(element('player').src, '/stream/' + sid, sid + ' Play URL');
    context.showSlide(Number(sid), 123);
    assert.equal(element('slideimg').src, 'slide/' + sid + '?cachebreak=123', sid + ' slide URL');
}
// Explicit string IDs remain explicit, without a second numeric conversion.
context.setPlayerSource('0x15dc');
assert.equal(element('player').src, '/stream/0x15dc');
console.log('PASS: Play and slide URLs retain explicit hexadecimal service IDs.');
