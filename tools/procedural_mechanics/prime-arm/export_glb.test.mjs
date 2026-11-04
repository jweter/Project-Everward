import test from 'node:test';
import assert from 'node:assert/strict';
import {mkdtempSync, readFileSync, rmSync} from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import {createHash} from 'node:crypto';
import {DESIGN} from './model.mjs';
import {MODULES} from './module_parts.mjs';
import {exportModules} from './export_glb.mjs';

function readGlb(file) {
  const data=readFileSync(file);
  assert.equal(data.toString('ascii',0,4),'glTF');
  assert.equal(data.readUInt32LE(4),2);
  assert.equal(data.readUInt32LE(8),data.length);
  const jsonLen=data.readUInt32LE(12);
  assert.equal(data.readUInt32LE(16),0x4E4F534A);
  const json=JSON.parse(data.toString('utf8',20,20+jsonLen));
  const binStart=20+jsonLen;
  assert.equal(data.readUInt32LE(binStart+4),0x004E4942);
  const binLen=data.readUInt32LE(binStart);
  assert.equal(binLen,json.buffers[0].byteLength);
  assert.equal(binStart+8+binLen,data.length);
  for(const view of json.bufferViews) {
    assert.ok(view.byteOffset>=0);
    assert.ok(view.byteOffset+view.byteLength<=binLen);
    assert.equal(view.byteOffset%4,0);
  }
  for(const acc of json.accessors) {
    assert.ok(acc.count>0);
    assert.ok(acc.bufferView>=0 && acc.bufferView<json.bufferViews.length);
  }
  return {data,json};
}
const checksum=bytes=>createHash('sha256').update(bytes).digest('hex');

test('exported normalized mesh catalog can be round-tripped and is repeatable',()=>{
  const tmp=mkdtempSync(path.join(os.tmpdir(),'everward-arm-glb-'));
  try {
    const m=exportModules(tmp);
    for(const type of ['modules','assembly']){
      const name='prime-arm-'+type+'.glb';
      const file=readGlb(path.join(tmp,name));
      assert.equal(checksum(file.data),m.products[name].sha256);
      assert.ok(file.data.length<512_000,'low-poly pilot must remain under 500 KB');
      assert.equal(file.json.meshes.length,3);
      assert.deepEqual(file.json.meshes.map(x=>x.name),Object.values(MODULES).map(x=>x.name));
      assert.ok(file.json.meshes.every(x=>x.primitives.length>=4));
      assert.ok(file.json.accessors.every(x=>x.count>0));
      assert.equal(file.json.scenes.length,1);
    }
    const a=readFileSync(path.join(tmp,'prime-arm-modules.glb'));
    exportModules(tmp);
    assert.deepEqual(readFileSync(path.join(tmp,'prime-arm-modules.glb')),a,
      'exact same inputs must produce byte-identical output');
  } finally {
    rmSync(tmp,{recursive:true,force:true});
  }
});

test('unit components are zero-origin independently importable assets',()=>{
  const tmp=mkdtempSync(path.join(os.tmpdir(),'everward-arm-parts-'));
  try{
    exportModules(tmp);
    const {json}=readGlb(path.join(tmp,'prime-arm-modules.glb'));
    const names=json.nodes.map(x=>x.name);
    for(const mod of Object.values(MODULES))assert.ok(names.includes(mod.name));
    assert.ok(json.nodes.filter(x=>x.mesh!==undefined).every(x=>!x.translation));
  }finally{rmSync(tmp,{recursive:true,force:true});}
});

test('assembly retains port/starboard joints, tool tips and true numeric lever arms',()=>{
  const tmp=mkdtempSync(path.join(os.tmpdir(),'everward-arm-assembly-'));
  try{
    exportModules(tmp);
    const {json}=readGlb(path.join(tmp,'prime-arm-assembly.glb'));
    const nodes=new Map(json.nodes.map(n=>[n.name,n]));
    assert.equal(nodes.get('EverwardProbeLocalToGltfYUp').rotation.length,4);
    for(const side of ['port','starboard']){
      const should=nodes.get('EW_'+side+'_ShoulderPivot');
      const elbow=nodes.get('EW_'+side+'_ElbowPivot');
      const wrist=nodes.get('EW_'+side+'_WristPivot');
      const tool=nodes.get('EW_'+side+'_MiningToolUnit');
      const tip=nodes.get('EW_'+side+'_AuthoritativeToolTip');
      assert.ok(should && elbow && wrist && tool && tip);
      assert.deepEqual(should.translation,DESIGN.shoulder_m[side]);
      assert.deepEqual(elbow.translation,[DESIGN.link_m.upper,0,0]);
      assert.deepEqual(wrist.translation,[DESIGN.link_m.forearm,0,0]);
      assert.deepEqual(tip.translation,[DESIGN.link_m.tool_tip_from_wrist,0,0]);
      assert.equal(tip.extras.collision_radius_m,DESIGN.sample_radius_m.tool_tip);
      assert.deepEqual(tool.scale,[0.34,0.34,0.82]);
    }
  }finally{rmSync(tmp,{recursive:true,force:true});}
});
