const assert = require('node:assert/strict');
const vm = require('node:vm');
const {read} = require('./qml-source.js');
const ctx = vm.createContext({PwLinkState:{Active:6},PwNodeType:{AudioSource:9}});
for (const m of read('AudioTopology.qml').matchAll(/^  function \w+\([^)]*\) \{[\s\S]*?^  \}/gm)) vm.runInContext(m[0],ctx);
const node = (name,props={},stream=false) => ({name,ready:true,isStream:stream,isSink:false,properties:props});
const mic = node('alsa_input.usb-ASUSTek_ROG_CETRA_TRUE_WIRELESS_SPEEDNOVA_0000.mono'); mic.type=9;
const other = node('alsa_input.laptop'); other.type=9;
const dsp = node('processor',{'media.role':'DSP'});
const virtual = node('processed_source');
const discord = node('WEBRTC VoiceEngine',{},true);
const record = node('recorder',{'media.role':'Production'},true);
const keep = node('pw-record',{'media.filename':'/dev/null'},true);
const peak = node('Cetra Peak Detect',{'media.role':'communication'},true);
const edge = (source,target,state=6) => ({source,target,state});
const observe = (nodes,edges) => JSON.parse(JSON.stringify(ctx.observe(mic,nodes,edges)));
assert.deepEqual(observe([mic,dsp,virtual,keep,peak],[edge(mic,dsp),edge(dsp,virtual),edge(virtual,keep),edge(mic,peak)]),{capture:'inactive',communication:'inactive'});
assert.deepEqual(observe([mic,dsp,virtual,discord],[edge(mic,dsp),edge(dsp,virtual),edge(virtual,discord)]),{capture:'active',communication:'active'});
assert.deepEqual(observe([mic,record],[edge(mic,record)]),{capture:'active',communication:'inactive'});
assert.deepEqual(observe([mic,other,discord],[edge(other,discord)]),{capture:'inactive',communication:'inactive'});
assert.deepEqual(observe([mic,dsp,discord],[edge(mic,dsp),edge(dsp,discord,5)]),{capture:'inactive',communication:'inactive'});
assert.deepEqual(observe([mic,other,virtual,discord],[edge(mic,virtual),edge(other,virtual),edge(virtual,discord)]),{capture:'unknown',communication:'unknown'});
assert.deepEqual(observe([mic,dsp,virtual,discord],[edge(mic,dsp),edge(dsp,dsp),edge(dsp,virtual),edge(virtual,discord)]),{capture:'active',communication:'active'});
assert.equal(ctx.observe(mic,Array(513).fill(mic),[]).capture,'unknown');
assert.equal(ctx.observe(null,[],[]).capture,'unknown');
assert.equal(ctx.isCommunication(node('Chromium input',{},true)),false);
assert.equal(ctx.isCommunication(node('Chromium input',{'media.role':'communication'},true)),true);
assert.equal(ctx.isCommunication(node('Discord recorder',{'media.role':'Production'},true)),false);
console.log('PASS topology: real routes, processors/keepalive exclusion, foreign/mixed mic, cycles, budget, call roles');

const address='AA:BB:CC:DD:EE:01';
const btProps={'device.api':'bluez5','api.bluez5.address':address,'api.bluez5.profile':'a2dp-sink','media.class':'Audio/Sink'};
const btOut=node('bluez_output.AA_BB_CC_DD_EE_01.1',btProps);btOut.isSink=true;
const btIn=node('bluez_input.AA_BB_CC_DD_EE_01.1',{...btProps,'media.class':'Audio/Source','api.bluez5.profile':'headset-head-unit'});
const usbOut=node('alsa_output.usb-ASUSTek_ROG_CETRA_TRUE_WIRELESS_SPEEDNOVA_0000.stereo');usbOut.isSink=true;
const player=node('music',{'media.role':'Music'},true);player.isSink=true;
const otherOut=node('alsa_output.laptop');otherOut.isSink=true;
assert.equal(ctx.route([player,dsp,btOut],[edge(player,dsp),edge(dsp,btOut)],address,true),'bluetooth');
assert.equal(ctx.route([player,usbOut,btOut],[edge(player,usbOut)],address,true),'usb','Connected BT/default hints are not actual routes');
assert.equal(ctx.route([player,usbOut,btOut],[edge(player,usbOut),edge(player,btOut)],address,true),'mixed');
assert.equal(ctx.route([player,otherOut],[edge(player,otherOut)],address,true),'other');
assert.equal(ctx.route([player,btOut],[edge(player,btOut,5)],address,true),'inactive');
assert.equal(ctx.route([player,dsp],[edge(player,dsp),edge(dsp,dsp)],address,true),'unknown');
assert.equal(ctx.route([player],[edge(player,null)],address,true),'unknown');
assert.equal(ctx.route([btIn,virtual,discord],[edge(btIn,virtual),edge(virtual,discord)],address,false),'bluetooth');
assert.equal(ctx.route([mic,btIn,virtual,discord],[edge(mic,virtual),edge(btIn,virtual),edge(virtual,discord)],address,false),'mixed');
assert.deepEqual(observe([mic,btIn,virtual,discord],[edge(mic,virtual),edge(btIn,virtual),edge(virtual,discord)]),{capture:'unknown',communication:'unknown'});
assert.equal(ctx.observe(mic,[mic,btIn,discord],[edge(btIn,discord)]).communication,'inactive');
assert.equal(ctx.audioState([btOut],[],address).profile,'a2dp');
assert.equal(ctx.audioState([btOut],[],address).microphone,false);
assert.equal(ctx.audioState([btIn],[],address).profile,'hfp');
assert.equal(ctx.audioState([btIn],[],address).microphone,true);
assert.equal(ctx.audioState([btIn,btOut],[],address).profile,'unknown','Conflicting metadata stays unknown');
assert.equal(ctx.audioState([btOut],[],'').profile,'unknown');
assert.equal(ctx.audioState([],[],address).profile,'unknown','No endpoint does not prove profile off');
console.log('PASS Bluetooth topology: active processed/mixed routes, profile evidence and USB call isolation');
// PipeWire 1.6.8 HFP: public loopback source lacks api.bluez5.address.
const hfpInternal=node('bluez_input.AA_BB_CC_DD_EE_01.0',{...btProps,'media.class':'Audio/Source/Internal','api.bluez5.profile':'headset-head-unit','device.id':128});
const hfpPublic=node('bluez_input.AA:BB:CC:DD:EE:01',{'media.class':'Audio/Source','bluez5.loopback':true,'device.id':'128'});
const hfpStream=node('bluez_capture_internal.AA:BB:CC:DD:EE:01',{'media.class':'Stream/Input/Audio/Internal'},true);
const hfpNodes=[hfpInternal,hfpPublic,hfpStream,discord];
assert.equal(ctx.audioState(hfpNodes,[],address).profile,'hfp');
assert.equal(ctx.audioState(hfpNodes,[],address).microphone,true);
assert.equal(ctx.route(hfpNodes,[edge(hfpPublic,discord)],address,false),'bluetooth');
assert.equal(ctx.route(hfpNodes,[edge(hfpInternal,hfpStream)],address,false),'inactive','Internal loopback alone is not application recording');
assert.equal(ctx.bluetoothNodeAddress(hfpPublic,[]),'','Name alone cannot relate the public source');
assert.equal(ctx.bluetoothNodeAddress(hfpPublic,[hfpInternal,{...hfpInternal,properties:{...hfpInternal.properties,'api.bluez5.address':'AA:BB:CC:DD:EE:02'}}]),'','Conflicting card association is unknown');
console.log('PASS HFP loopback: explicit device.id relation, public microphone endpoint, no internal-stream capture claim');

assert.equal(ctx.route([hfpPublic,discord],[edge(hfpPublic,discord)],address,false),'unknown','Unresolved BlueZ identity is not another proven device');

// Dictation is capture, never a communication call (including explicit roles).
const dictation = node('alsa_capture.voxtype-vulkan', {
  'application.name':'PipeWire ALSA [voxtype-vulkan]', 'media.name':'ALSA Capture'
}, true);
const dictationActive = {capture:'active',communication:'inactive'};
const dictationIdle = {capture:'inactive',communication:'inactive'};
assert.deepEqual(observe([mic,dictation],[edge(mic,dictation)]),dictationActive);
assert.deepEqual(observe([mic,dsp,virtual,dictation],
  [edge(mic,dsp),edge(dsp,virtual),edge(virtual,dictation)]),dictationActive);
for (const edges of [[], [edge(mic,dictation,5)], [edge(other,dictation)]])
  assert.deepEqual(observe([mic,other,dictation],edges),dictationIdle);
for (const corked of [true,'true']) {
  const paused = {...dictation,properties:{...dictation.properties,'pulse.corked':corked}};
  assert.deepEqual(observe([mic,paused],[edge(mic,paused)]),dictationIdle);
}
for (const name of ['alsa_capture.voxtype-vulkan','speech recognition']) {
  const tagged = node(name,{'media.role':'communication'},true);
  assert.equal(ctx.isCommunication(tagged),false);
  assert.deepEqual(observe([mic,tagged],[edge(mic,tagged)]),dictationActive);
}
const dictationKeep = {...dictation,properties:{...dictation.properties,'media.name':'/dev/null'}};
assert.deepEqual(observe([mic,dictationKeep,peak],[edge(mic,dictationKeep),edge(mic,peak)]),dictationIdle);
assert.deepEqual(observe([mic,other,virtual,dictation],
  [edge(mic,virtual),edge(other,virtual),edge(virtual,dictation)]),{capture:'unknown',communication:'unknown'});
console.log('PASS dictation: direct/processed capture, no call, end/pause/other mic/mixed mic, keepalive/self exclusion');
