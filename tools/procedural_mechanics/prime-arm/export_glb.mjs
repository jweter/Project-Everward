#!/usr/bin/env node
/**
 * Dependency-free, deterministic glTF 2.0 binary exporter for the Everward
 * mechanical bridge. Produces:
 *   prime-arm-modules.glb  — THREE normalized, zero-origin mesh components
 *   prime-arm-assembly.glb — both named, pivoted arms in an illustrative pose
 *   prime-arm-manifest.json — importer mapping + SHA256 provenance
 *
 * Run: node tools/procedural_mechanics/prime-arm/export_glb.mjs <output-directory>
 * No Unreal Editor, Blender, npm install, network or Iron God is touched.
 */
import {mkdirSync, writeFileSync} from 'node:fs';
import {createHash} from 'node:crypto';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {DESIGN, forwardKinematics} from './model.mjs';
import {PALETTE, MODULES} from './module_parts.mjs';

const D = Math.PI / 180;
const qx = (r) => [Math.sin(r / 2), 0, 0, Math.cos(r / 2)];
const qy = (r) => [0, Math.sin(r / 2), 0, Math.cos(r / 2)];
function qmul(a, b) {
  const [x,y,z,w] = a, [X,Y,Z,W] = b;
  return [w*X+x*W+y*Z-z*Y, w*Y-x*Z+y*W+z*X,
    w*Z+x*Y-y*X+z*W, w*W-x*X-y*Y-z*Z];
}
const pad = (n) => (n + 3) & ~3;
const bufFloat = (numbers) => {
  const b = Buffer.alloc(numbers.length * 4);
  numbers.forEach((v,i) => b.writeFloatLE(v,i*4));
  return b;
};
const bufUint = (numbers) => {
  const b = Buffer.alloc(numbers.length * 4);
  numbers.forEach((v,i) => b.writeUInt32LE(v,i*4));
  return b;
};

function box(part) {
  const [x,y,z] = part.center, [sx,sy,sz] = part.size.map(v => v / 2);
  const faces = [
    [[1,0,0],[[1,-1,-1],[1,1,-1],[1,1,1],[1,-1,1]]],
    [[-1,0,0],[[-1,1,-1],[-1,-1,-1],[-1,-1,1],[-1,1,1]]],
    [[0,1,0],[[1,1,-1],[-1,1,-1],[-1,1,1],[1,1,1]]],
    [[0,-1,0],[[-1,-1,-1],[1,-1,-1],[1,-1,1],[-1,-1,1]]],
    [[0,0,1],[[1,-1,1],[1,1,1],[-1,1,1],[-1,-1,1]]],
    [[0,0,-1],[[-1,-1,-1],[-1,1,-1],[1,1,-1],[1,-1,-1]]],
  ];
  const positions=[], normals=[], indices=[];
  for(const [normal, corners] of faces) {
    const start=positions.length/3;
    for(const c of corners) {
      positions.push(x + c[0]*sx,y + c[1]*sy,z + c[2]*sz);
      normals.push(...normal);
    }
    indices.push(start,start+1,start+2,start,start+2,start+3);
  }
  return {positions,normals,indices};
}

function cylinder(part) {
  const n=part.segments;
  if (!Number.isInteger(n) || n<6 || n>64) throw new Error('Invalid segments');
  const positions=[],normals=[],indices=[];
  const [x,y,z]=part.center;
  const bottomZ=z-part.height/2, topZ=z+part.height/2;
  const slope=(part.bottom-part.top)/part.height;
  const unit=Math.hypot(1,slope);
  for(let i=0;i<n;i++) {
    const a=2*Math.PI*i/n, b=2*Math.PI*(i+1)/n;
    const coords=[
      [part.bottom*Math.cos(a),part.bottom*Math.sin(a),bottomZ],
      [part.bottom*Math.cos(b),part.bottom*Math.sin(b),bottomZ],
      [part.top*Math.cos(b),part.top*Math.sin(b),topZ],
      [part.top*Math.cos(a),part.top*Math.sin(a),topZ],
    ];
    const start=positions.length/3;
    coords.forEach(([px,py,pz],j)=>{
      positions.push(x+px,y+py,pz);
      const angle=(j===1||j===2)?b:a;
      normals.push(Math.cos(angle)/unit,Math.sin(angle)/unit,slope/unit);
    });
    indices.push(start,start+1,start+2,start,start+2,start+3);
  }
  for(const [r,h,sign] of [[part.bottom,bottomZ,-1],[part.top,topZ,1]]) {
    const start=positions.length/3;
    positions.push(x,y,h); normals.push(0,0,sign);
    for(let i=0;i<n;i++) {
      const a=2*Math.PI*i/n;
      positions.push(x+r*Math.cos(a),y+r*Math.sin(a),h);
      normals.push(0,0,sign);
    }
    for(let i=0;i<n;i++) {
      const a=start+1+i,b=start+1+(i+1)%n;
      if(sign>0)indices.push(start,a,b);
      else indices.push(start,b,a);
    }
  }
  return {positions,normals,indices};
}

function buildGlb(mode) {
  const gltf={
    asset:{version:'2.0',generator:'Everward Mechanical Bridge v0.1 (original)'},
    scene:0,scenes:[{name:mode==='assembly'?'Everward_3D_Arm_Assembly':'Everward_Arm_Unit_Modules',nodes:[]}],
    nodes:[],meshes:[],materials:[],buffers:[{byteLength:0}],bufferViews:[],accessors:[],
  };
  const blobs=[];
  let byteLength=0;
  function writeView(buffer,target) {
    const start=byteLength;
    gltf.bufferViews.push({buffer:0,byteOffset:start,byteLength:buffer.length,target});
    blobs.push(buffer);
    byteLength += buffer.length;
    const extra=pad(byteLength)-byteLength;
    if(extra){blobs.push(Buffer.alloc(extra));byteLength+=extra;}
    return gltf.bufferViews.length-1;
  }
  function accessor(values,type,componentType,target,includeBounds=false) {
    const buffer=componentType===5126?bufFloat(values):bufUint(values);
    const index=gltf.accessors.length;
    const itemCount=type==='VEC3'?3:1;
    const entry={bufferView:writeView(buffer,target),componentType,type,count:values.length/itemCount};
    if(includeBounds) {
      entry.min=Array(itemCount).fill(Infinity);
      entry.max=Array(itemCount).fill(-Infinity);
      for(let i=0;i<values.length;i++) {
        const k=i%itemCount;
        entry.min[k]=Math.min(entry.min[k],values[i]);
        entry.max[k]=Math.max(entry.max[k],values[i]);
      }
    }
    gltf.accessors.push(entry);
    return index;
  }
  const materialNames=Object.keys(PALETTE);
  materialNames.forEach(name=>{
    const p=PALETTE[name];
    gltf.materials.push({
      name:'Everward_'+name,
      pbrMetallicRoughness:{
        baseColorFactor:p.baseColor,metallicFactor:p.metallic,roughnessFactor:p.roughness,
      },
      doubleSided:false,
    });
  });
  const meshByName={};
  for (const [key,mod] of Object.entries(MODULES)) {
    const primitives=[];
    for (const spec of mod.parts) {
      const geo=spec.kind==='box'?box(spec):cylinder(spec);
      primitives.push({
        attributes:{
          POSITION:accessor(geo.positions,'VEC3',5126,34962,true),
          NORMAL:accessor(geo.normals,'VEC3',5126,34962),
        },
        indices:accessor(geo.indices,'SCALAR',5125,34963),
        material:materialNames.indexOf(spec.material),
        mode:4,
      });
    }
    meshByName[key]=gltf.meshes.length;
    gltf.meshes.push({name:mod.name,primitives});
  }

  function node(name, options={}, parent=null) {
    const id=gltf.nodes.length;
    gltf.nodes.push({name,...options});
    if(parent===null)gltf.scenes[0].nodes.push(id);
    else (gltf.nodes[parent].children ??= []).push(id);
    return id;
  }
  if(mode==='modules'){
    const root=node('EW_UNIT_COMPONENTS_NO_WORLD_TRANSFORM');
    for (const [key,mod] of Object.entries(MODULES)) {
      node(mod.name,{mesh:meshByName[key],extras:{axis:mod.axis,normalized:true}},root);
    }
  } else {
    // Native engineering coordinates are UE X-forward, Y-starboard, Z-up.
    // Convert the *whole* scene to glTF's Y-up convention at the root.
    const root=node('EverwardProbeLocalToGltfYUp',{rotation:qx(-Math.PI/2)});
    for(const arm of ['port','starboard']){
      const p=forwardKinematics({arm,deploymentFraction:1,anglesDegrees:{shoulder:0,elbow:45,wrist:-20}});
      const sign=arm==='port'?-1:1;
      const shoulderQ=qmul(
        qy((DESIGN.fold_pitch_degrees.deployed)*D),
        qx(sign*DESIGN.fold_roll_degrees.deployed*D),
      );
      const should=node('EW_'+arm+'_ShoulderPivot',{
        translation:p.shoulder,rotation:shoulderQ,
      },root);
      node('EW_'+arm+'_UpperUnit',{
        mesh:meshByName.upper,
        translation:[DESIGN.link_m.upper/2,0,0],scale:[DESIGN.link_m.upper,0.18,0.18],
      },should);
      const elbow=node('EW_'+arm+'_ElbowPivot',{
        translation:[DESIGN.link_m.upper,0,0],rotation:qy(45*D),
      },should);
      node('EW_'+arm+'_ForearmUnit',{
        mesh:meshByName.forearm,
        translation:[DESIGN.link_m.forearm/2,0,0],scale:[DESIGN.link_m.forearm,0.16,0.16],
      },elbow);
      const wrist=node('EW_'+arm+'_WristPivot',{
        translation:[DESIGN.link_m.forearm,0,0],rotation:qy(-20*D),
      },elbow);
      node('EW_'+arm+'_MiningToolUnit',{
        mesh:meshByName.tool,
        translation:[0.38,0,0],rotation:qy(Math.PI/2),
        scale:[0.34,0.34,0.82],
      },wrist);
      node('EW_'+arm+'_AuthoritativeToolTip',{
        translation:[DESIGN.link_m.tool_tip_from_wrist,0,0],
        extras:{marker_only:true,collision_radius_m:DESIGN.sample_radius_m.tool_tip},
      },wrist);
    }
  }
  gltf.buffers[0].byteLength=byteLength;
  const jsonBytes=Buffer.from(JSON.stringify(gltf),'utf8');
  const jsonPad=Buffer.alloc(pad(jsonBytes.length)-jsonBytes.length,0x20);
  const bin=Buffer.concat(blobs);
  const total=12+8+jsonBytes.length+jsonPad.length+8+bin.length;
  const header=Buffer.alloc(12);
  header.write('glTF',0,4,'ascii');
  header.writeUInt32LE(2,4);header.writeUInt32LE(total,8);
  const jsonChunk=Buffer.alloc(8);
  jsonChunk.writeUInt32LE(jsonBytes.length+jsonPad.length,0);
  jsonChunk.writeUInt32LE(0x4E4F534A,4);
  const binChunk=Buffer.alloc(8);
  binChunk.writeUInt32LE(bin.length,0);
  binChunk.writeUInt32LE(0x004E4942,4);
  return Buffer.concat([header,jsonChunk,jsonBytes,jsonPad,binChunk,bin]);
}

export function exportModules(outputDir) {
  mkdirSync(outputDir,{recursive:true});
  const manifest={schema_version:1,generator:'original-everward-mechanical-bridge-v01',
    authority:'src/simulation/include/everward/simulation/manipulator_tool_contact.hpp',
    runtime_truth:'authoritative C++ only',
    unit_scale:'One glTF metre = 100 Unreal centimetres',
    cxx_contract_sha_hint:'must pass bridge_contract.test.mjs on exact commit',
    normalised_modules:{
      upper:{asset_name:MODULES.upper.name,attach_component:'PortUpperArm or StarboardUpperArm',
        local_axis:'X',component_scale_ue:[1.8,0.18,0.18],component_location_ue_cm:[90,0,0]},
      forearm:{asset_name:MODULES.forearm.name,attach_component:'PortForearm or StarboardForearm',
        local_axis:'X',component_scale_ue:[1.5,0.16,0.16],component_location_ue_cm:[75,0,0]},
      tool:{asset_name:MODULES.tool.name,attach_component:'PortToolHead or StarboardToolHead',
        local_axis:'Z',component_location_ue_cm:[38,0,0],component_rotation_ue_deg:[90,0,0],
        attached_scale_ue:[0.34,0.34,0.82],detached_scale_ue:[0.24,0.24,0.56]},
    },
    products:{},claimed_unreal_verified:false,
  };
  for(const mode of ['modules','assembly']){
    const name='prime-arm-'+mode+'.glb';
    const bytes=buildGlb(mode);
    writeFileSync(path.join(outputDir,name),bytes);
    manifest.products[name]={bytes:bytes.length,sha256:createHash('sha256').update(bytes).digest('hex')};
  }
  writeFileSync(path.join(outputDir,'prime-arm-manifest.json'),JSON.stringify(manifest,null,2)+'\n');
  return manifest;
}

if(process.argv[1] && path.resolve(process.argv[1])===fileURLToPath(import.meta.url)){
  if(process.argv.length!==3) {
    console.error('usage: node export_glb.mjs <output-dir> (writes only that directory)');
    process.exit(2);
  }
  const result=exportModules(path.resolve(process.argv[2]));
  process.stdout.write(JSON.stringify(result.products)+'\n');
}
