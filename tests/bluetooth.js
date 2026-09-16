// Production functions and bindings. Qt signal ordering is covered separately.
const assert = require('node:assert/strict');
const vm = require('node:vm');
const {read} = require('./qml-source.js');
function functions(file, ctx) {
  for (const m of read(file).matchAll(/^  function \w+\([^)]*\) \{[\s\S]*?^  \}/gm)) vm.runInContext(m[0], ctx);
}
const ctx = vm.createContext({}); functions('CetraBluetooth.qml', ctx);
const a = 'AA:BB:CC:DD:EE:01', b = 'AA:BB:CC:DD:EE:02';
const device = (address, name = 'ROG CETRA TWS SN') => ({ address, name, connected:true, batteryAvailable:true, battery:0.55 });
const audio = address => ({ready:true,isStream:false,properties:{'device.api':'bluez5','media.class':'Audio/Sink','api.bluez5.address':address}});
const main = device(a), le = device(b, 'LE-ROG CTWSN'); le.battery = .71;
assert.equal(ctx.resolveDevice([main,le], ''), null, 'Never auto-select by name');
assert.equal(ctx.resolveDevice([main,le], a.toLowerCase()), main);
assert.equal(ctx.resolveDevice([main,{...main}],a),null,'Duplicate identities stay ambiguous');
assert.deepEqual(JSON.parse(JSON.stringify(ctx.audioCandidates([le,main], [audio(a)]))), [{address:a,name:main.name}]);
assert.equal(ctx.audioCandidates([main], []).length,0,'Name alone cannot admit an audio candidate');
assert.equal(ctx.audioCandidates(Array(65).fill(main), [audio(a)]).length,0);
assert.equal(ctx.audioCandidates([main], Array(513).fill(audio(a))).length,0);
for (const bad of [null, undefined, '0.5', false, NaN, Infinity, -1, 1.01, 55]) assert.equal(ctx.batteryPercent(true,bad),null);
assert.equal(ctx.batteryPercent(false,.55),null);
assert.equal(ctx.batteryPercent(true,0),0);
assert.equal(ctx.batteryPercent(true,1),100);
ctx.device = main; ctx.audioConnected = true; ctx.generation = 0;
ctx.resetEvidence(); assert.equal(ctx.battery.percent,55); assert.equal(ctx.battery.snapshot,'cached');
assert.equal(ctx.battery.hardwareReportedAt,null); assert.equal(ctx.battery.observedAt,null);
const old = ctx.generation;
main.battery = .60; ctx.acceptBattery(main,old,true); assert.equal(ctx.battery.snapshot,'property-update');
ctx.audioConnected = false; ctx.resetEvidence(); assert.equal(ctx.battery.percent,null);
ctx.acceptBattery(main,old,true); assert.equal(ctx.battery.percent,null);
ctx.audioConnected = true; ctx.resetEvidence(); assert.equal(ctx.battery.snapshot,'cached');
ctx.acceptBattery(le,ctx.generation,true); assert.equal(ctx.battery.percent,60,'LE cannot overwrite main charge');
const replacement = device(a); ctx.device=replacement; ctx.resetEvidence();
main.battery = .9; ctx.acceptBattery(main,ctx.generation,true); assert.equal(ctx.battery.percent,55,'Old object cannot update replacement');
ctx.device = null; ctx.audioConnected = null; ctx.resetEvidence(); assert.equal(ctx.battery.percent,null);

const view = vm.createContext({}); view.root=view; functions('CetraViewModel.qml',view);
function bind(name) {const line=read('CetraViewModel.qml').match(new RegExp(`^  readonly property \\w+ ${name}: (.+)$`,'m'))[1];Object.defineProperty(view,name,{get:()=>vm.runInContext(line,view)});}
bind('hideWhenReceiverMissing');bind('hideWhenDisconnected');bind('panelAvailable');
view.service = {settings:{}}; assert.equal(view.hideWhenDisconnected,true);
view.service.settings={hideWhenReceiverMissing:false};assert.equal(view.hideWhenDisconnected,false);
view.service.settings={hideWhenReceiverMissing:false,hideWhenDisconnected:true,untouched:'yes'};assert.equal(view.hideWhenDisconnected,true);assert.equal(view.service.settings.untouched,'yes');
for(const usb of [true,false])for(const bt of [true,false,null]) {view.receiver=usb;view.bluetoothAudioConnected=bt;assert.equal(view.panelAvailable,usb||bt===true);}
assert.doesNotMatch(read('CetraBluetooth.qml'), /\b(Timer|Process)\s*\{|\.(connect|disconnect|pair|forget)\(|\.(connected|trusted|discovering)\s*=/);
assert.match(read('CetraService.qml'),/CallDetector \{[^\n]*audioTopology.observation.communication/);
assert.match(read('AudioTopology.qml'),/active && usbAvailable && Pipewire.ready/);
console.log('PASS Bluetooth: explicit identity, candidate bounds, battery provenance/generations, visibility and preference migration');
const svc = vm.createContext({bluetoothDiagnosticsEnabled:true,bluetoothDiagnostics:[],bluetoothDiagnosticState:'initial'});
functions('CetraService.qml',svc);
for(let i=0;i<30;i++){svc.bluetoothDiagnosticState=String(i);svc.recordBluetoothTransition();svc.recordBluetoothTransition();}
assert.equal(svc.bluetoothDiagnostics.length,16);
assert.equal(svc.bluetoothDiagnostics[0],'14');
svc.bluetoothDiagnosticsEnabled=false;svc.recordBluetoothTransition();assert.equal(svc.bluetoothDiagnostics.length,0);
const settings={custom:'preserved',hideWhenReceiverMissing:false};
svc.bluetoothObserver=ctx;svc.bluetoothCandidates=[{address:a}];svc.updateSetting=(key,value)=>{settings[key]=value;return true;};
assert.equal(svc.selectBluetoothAudio(b),false);
assert.equal(svc.selectBluetoothAudio(null),false);
assert.equal(svc.selectBluetoothAudio(a.toLowerCase()),true);
assert.equal(settings.bluetoothAudioAddress,a);
assert.equal(settings.custom,'preserved');
assert.equal(settings.hideWhenReceiverMissing,false);
assert.equal(svc.selectBluetoothAudio(''),true);
assert.equal(settings.bluetoothAudioAddress,'');
console.log('PASS Bluetooth diagnostics: bounded/deduplicated/opt-out; selection admits only observed audio candidates');
