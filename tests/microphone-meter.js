// Actual production gate and parser functions; no audio capture.
const assert = require('node:assert/strict');
const vm = require('node:vm');
const {read} = require('./qml-source.js');
const source = read('MicrophoneMeter.qml');
const ctx = vm.createContext({PwLinkState: {Active: 6}, PwNodeType: {AudioSource: 9}});
for (const match of (source+'\n'+read('AudioTopology.qml')).matchAll(/^  function \w+\([^)]*\) \{[\s\S]*?^  \}/gm)) vm.runInContext(match[0], ctx);
const mic = {isStream:false, isSink:false, type:9, name:'alsa_input.usb-ASUSTek_ROG_CETRA_TRUE_WIRELESS_SPEEDNOVA_0000-00.mono-fallback'};
const other = {...mic, name:'alsa_input.laptop'};
assert.equal(ctx.selectSource([other,mic]), mic);
assert.equal(ctx.selectSource([other]), null);
assert.equal(ctx.selectSource([mic,{...mic}]), null);
assert.equal(ctx.selectSource([{...mic,isSink:true},{...mic,isStream:true}]), null);
assert.equal(ctx.selectSource([{...mic,type:0}]), null);
const app = {ready:true, name:'WEBRTC VoiceEngine', properties:{'application.name':'Discord'}};
const link = target => ({source:mic, target, state:6});
mic.ready = true;
app.isStream = true; app.isSink = false;
ctx.hasExternalCapture = (m, edges) => ctx.observe(m,[m, ...edges.map(e=>e.target)].filter(Boolean),edges).capture === 'active';
assert.equal(ctx.hasExternalCapture(mic,[link(app)]), true);
assert.equal(ctx.hasExternalCapture(mic,[{...link(app),source:other}]), false);
assert.equal(ctx.hasExternalCapture(mic,[{...link(app),state:5}]), false);
assert.equal(ctx.hasExternalCapture(mic,[link({...app,ready:false})]), false);
for (const props of [{'application.name':'Quickshell Peak Detect'}, {'media.name':'Peak detect'},
  {'application.name':'Cetra Peak Detect'}, {'application.id':'io.github.pavellizunov.rog-cetra-control.peak'},
  {'stream.monitor':true}, {'stream.monitor':'true'}, {'media.category':'Monitor'}]) {
  const own = {...app, properties:props};
  assert.equal(ctx.hasExternalCapture(mic,[link(own)]), false);
  assert.equal(ctx.hasExternalCapture(mic,[link(own),link(app)]), true);
}
assert.equal(ctx.hasExternalCapture(mic,[link({...app,name:'quickshell-peak-monitor'})]), false);
assert.equal(ctx.hasExternalCapture(null,[link(app)]), false);
for (const invalid of [null,undefined,'0.5',NaN,Infinity,{}]) assert.equal(ctx.boundedPeak(invalid),null);
for (const [n,want] of [[-1,0],[0,0],[0.3,0.3],[2,1]]) assert.equal(ctx.boundedPeak(n),want);
assert.match(source, /Process \{\s*id: peaks/);
assert.match(source, /id: retry\s*interval: 2000/);
assert.doesNotMatch(source, /repeat: true/);
assert.match(source, /onInUseChanged: reconcile\(\)/);
assert.match(source, /topology.observation.capture === "active"/);
for (const line of ['bad','null','{}','{"level":null}','{"level":"0.2"}','{"level":1e999}',' '.repeat(129)])
  assert.equal(ctx.parseLevel(line),null);
assert.equal(ctx.parseLevel('{"level":0}'),0);
assert.equal(ctx.parseLevel('{"level":0.4}'),0.4);
ctx.inUse = false; ctx.level = 0.5; ctx.peaks = {running:true,capturedSource:mic.name}; ctx.sourceName = mic.name;
ctx.retry = {stop(){},start(){}};
ctx.stopDeadline = {restart(){}};
ctx.reconcile(); assert.equal(ctx.level,null); assert.equal(ctx.peaks.running,false);
ctx.inUse = true; ctx.peaks.running = true; ctx.sourceName = other.name;
ctx.reconcile(); assert.equal(ctx.peaks.running,false);
assert.doesNotMatch(source, /FileView|\.write\(|\.muted\s*=|move-source-output|PwNodePeakMonitor/);
assert.doesNotMatch(read('daemon/reports.h'), /outside call -> media play\/pause/);
assert.match(source, /peaks.command = \[meter.peakCommand, meter.sourceName\]/);
assert.doesNotMatch(read('MicrophoneLevel.qml'),/Repeater|NumberAnimation/);
assert.match(read('MicrophoneLevel.qml'),/implicitWidth: Style.space\(3\)/);
const manifest = JSON.parse(read('manifest.json'));
assert.equal(manifest.barWidget.defaults.showMicLevel, false);
assert.equal(manifest.barWidget.schema.find(f=>f.key==='showMicLevel').defaultValue,false);
assert.doesNotMatch(read('CallDetector.qml'), /Process|pactl|jq|repeat: true/);
assert.equal(ctx.isEndpoint({...app,properties:{'application.name':'Cetra Peak Detect','media.role':'communication'}}), false);
const battery = read('BatterySection.qml');
const b = vm.createContext({modelData:{present:true,value:null}, root:{tr:(_,v)=>v,batteryStatusText:()=>''}});
const opacity = battery.match(/^\s*opacity: (.+)$/m)[1];
assert.equal(vm.runInContext(opacity,b),1);
const label = battery.match(/^\s*text: (modelData.present === true[\s\S]*?)^\s*horizontalAlignment:/m)[1];
assert.equal(vm.runInContext(label,b),'Available; battery unknown');
b.modelData.present = false;
assert.equal(vm.runInContext(opacity,b),0.4);
console.log('PASS microphone meter: exact source, external active links, self-exclusion, null/zero distinction, call filter and presence-only battery');

// Execute first admission and cooldown without launching audio processes.
ctx.meter = ctx;
ctx.retry = {running:false, restart(){this.running=true;}};
ctx.stopDeadline = {stop(){},restart(){}};
ctx.inUse=true; ctx.sourceName=mic.name; ctx.peakCommand='/fixture/cetra-peak';
ctx.peaks={running:false,capturedSource:'',command:[]};
ctx.reconcile();
assert.equal(ctx.peaks.running,true,'First verified capture starts immediately');
assert.equal(ctx.peaks.capturedSource,mic.name);
assert.deepEqual(Array.from(ctx.peaks.command),[ctx.peakCommand,mic.name]);
ctx.peaks.running=false;ctx.meterStopped();
ctx.reconcile();assert.equal(ctx.peaks.running,false,'Failure backoff blocks immediate restart');
ctx.inUse=false;ctx.reconcile();ctx.inUse=true;ctx.reconcile();
assert.equal(ctx.peaks.running,false,'Capture toggles cannot bypass failure cooldown');
ctx.retry.running=false;ctx.startPeaks();assert.equal(ctx.peaks.running,true);
ctx.sourceName=other.name;ctx.reconcile();assert.equal(ctx.peaks.running,false);
assert.equal(ctx.peaks.capturedSource,mic.name,'Stop old source before replacement');
ctx.meterStopped();ctx.retry.running=false;ctx.startPeaks();
assert.equal(ctx.peaks.capturedSource,other.name);
ctx.peaks.running=false;ctx.sourceName='';ctx.retry.running=false;ctx.reconcile();
assert.equal(ctx.peaks.running,false,'Missing source never starts helper');
console.log('PASS meter admission: immediate start, failure backoff, rapid capture toggle, source change');

// User-visible states use production functions; Bluetooth must never show USB PCM.
const panel = vm.createContext({connected:true, showMicLevel:true,
  microphoneCaptureState:'inactive',microphoneLevel:0.5,
  tr:(_key,fallback,values={})=>fallback.replace(/\{(\w+)\}/g,(_m,k)=>values[k])});
panel.root=panel;
for (const match of read('CetraViewModel.qml').matchAll(/^  function microphone\w+\([^)]*\) \{[\s\S]*?^  \}/gm)) vm.runInContext(match[0],panel);
assert.equal(panel.microphoneSignalState(),'idle','Stale numeric level cannot override inactive capture');
panel.microphoneCaptureState='unknown';assert.equal(panel.microphoneSignalState(),'unavailable');
panel.microphoneCaptureState='active';panel.microphoneLevel=null;
assert.equal(panel.microphoneSignalState(),'waiting');
for(const invalid of [NaN,Infinity,-1,'0.5']) {
 panel.microphoneLevel=invalid;assert.equal(panel.microphoneSignalState(),'waiting');
}
panel.microphoneLevel=0;assert.equal(panel.microphoneSignalState(),'silent');
assert.equal(panel.microphoneLevelText(),'No signal · 0%');
panel.microphoneLevel=.25;assert.equal(panel.microphoneLevelText(),'Signal: 25%');
panel.microphoneLevel=2;assert.equal(panel.microphoneLevelText(),'Signal: 100%');
panel.showMicLevel=false;assert.equal(panel.microphoneSignalState(),'disabled');
panel.connected=false;panel.showMicLevel=true;
assert.equal(panel.microphoneSignalState(),'usb-only');
assert.equal(panel.microphoneLevelText(),'Signal level requires USB');
assert.match(panel.microphoneHelpText(),/Microphone mute: unknown/);
console.log('PASS panel signal: inactive, unknown, awaiting data, zero, signal, disabled, Bluetooth USB-only');
