/**
 * Everward mechanical bridge v0.1 — dual-arm 3D engineering studio.
 * Explicitly NOT a simulation actor, certified physics or imported Unreal art.
 * Built by the pinned elvarofficial/teardown kit after isolated tool review:
 * node .tools/teardown/sims/build.mjs ./tools/procedural_mechanics/prime-arm
 */
import * as THREE from 'three';
import {runSim} from 'teardown-kit/runtime.js';
import {DESIGN, poseAt} from './model.mjs';
import {MODULES, PALETTE} from './module_parts.mjs';

const RAD = Math.PI/180;
const SETTINGS={
  duration:24,fps:30,theme:'light',
  hud:{enabled:false,title:'EVERWARD',subtitle:'MECHANICAL BRIDGE V0.1'},
  director:{enabled:false},
};
const PAINT=Object.fromEntries(Object.entries(PALETTE).map(([key,p])=>
  [key,new THREE.MeshStandardMaterial({
    color:new THREE.Color(...p.baseColor.slice(0,3)),
    metalness:p.metallic,roughness:p.roughness,
  })]));
function meshPart(p) {
  let geo;
  if(p.kind==='box') geo=new THREE.BoxGeometry(...p.size);
  else {
    geo=new THREE.CylinderGeometry(p.top,p.bottom,p.height,p.segments,1);
    // Three CylinderGeometry axis is Y; Everward toolhead local axis is Z.
    geo.rotateX(Math.PI/2);
  }
  const mesh=new THREE.Mesh(geo,PAINT[p.material]);
  mesh.position.set(...p.center);
  return mesh;
}
function component(key) {
  const module=new THREE.Group();
  module.name=MODULES[key].name;
  MODULES[key].parts.forEach((p,i)=>{
    const child=meshPart(p);
    child.name=module.name+'_detail_'+String(i+1).padStart(2,'0');
    module.add(child);
  });
  return module;
}
function makeArm(stage,side) {
  const shoulder=new THREE.Group();
  shoulder.name='EW_'+side+'_ShoulderPivot';
  stage.add(shoulder);
  const mount=new THREE.Mesh(
    new THREE.SphereGeometry(0.30,12,8),PAINT.rib);
  mount.name='EW_'+side+'_ShoulderHousing';
  shoulder.add(mount);

  const upper=component('upper');
  upper.position.set(DESIGN.link_m.upper/2,0,0);
  upper.scale.set(DESIGN.link_m.upper,0.18,0.18);
  shoulder.add(upper);
  const elbow=new THREE.Group();
  elbow.name='EW_'+side+'_ElbowPivot';
  elbow.position.x=DESIGN.link_m.upper;
  shoulder.add(elbow);
  const jointRing=new THREE.Mesh(new THREE.CylinderGeometry(0.19,0.19,0.30,12),PAINT.trim);
  jointRing.rotation.x=Math.PI/2;
  elbow.add(jointRing);

  const forearm=component('forearm');
  forearm.position.set(DESIGN.link_m.forearm/2,0,0);
  forearm.scale.set(DESIGN.link_m.forearm,0.16,0.16);
  elbow.add(forearm);
  const wrist=new THREE.Group();
  wrist.name='EW_'+side+'_WristPivot';
  wrist.position.x=DESIGN.link_m.forearm;
  elbow.add(wrist);
  const wristRing=new THREE.Mesh(new THREE.SphereGeometry(0.17,12,8),PAINT.trim);
  wrist.add(wristRing);

  const tool=component('tool');
  tool.position.set(0.38,0,0);
  tool.rotation.y=Math.PI/2;
  tool.scale.set(0.34,0.34,0.82);
  wrist.add(tool);
  const marker=new THREE.Mesh(new THREE.SphereGeometry(0.045,10,8),PAINT.trim);
  marker.name='EW_'+side+'_ToolTip_Contact_Marker';
  marker.position.x=DESIGN.link_m.tool_tip_from_wrist;
  wrist.add(marker);
  return {shoulder,elbow,wrist,upper,forearm,tool,marker};
}
function setMatrixQuaternion(object,m) {
  const mat=new THREE.Matrix4().set(
    m[0],m[1],m[2],0,
    m[3],m[4],m[5],0,
    m[6],m[7],m[8],0,
    0,0,0,1,
  );
  object.quaternion.setFromRotationMatrix(mat);
}
function panel() {
  const el=document.createElement('div');
  el.setAttribute('role','status');
  el.style.cssText=[
    'position:fixed','z-index:9','left:14px','bottom:14px',
    'background:rgba(18,29,34,.86)','border:1px solid #6f8185',
    'color:#e9f0f0','font:12px/1.5 monospace','padding:11px 15px',
    'white-space:pre','pointer-events:none',
  ].join(';');
  document.body.append(el);
  return el;
}

runSim(SETTINGS,async({canvas,capture})=>{
  const scene=new THREE.Scene();
  scene.background=new THREE.Color(0xe8eced);
  const renderer=new THREE.WebGLRenderer({
    canvas,antialias:false,preserveDrawingBuffer:true,powerPreference:'low-power',
  });
  renderer.setPixelRatio(1);
  renderer.outputColorSpace=THREE.SRGBColorSpace;
  const camera=new THREE.PerspectiveCamera(38,1,.1,120);
  camera.up.set(0,0,1);
  scene.add(new THREE.HemisphereLight(0xffffff,0x617078,2.0));
  const key=new THREE.DirectionalLight(0xffffff,2.8);
  key.position.set(5,4,9);scene.add(key);
  const fill=new THREE.DirectionalLight(0xadc0d7,1.0);
  fill.position.set(-5,-6,2);scene.add(fill);
  const stage=new THREE.Group();
  stage.name='EV-0001_ProbeLocal_Arm_Study';
  scene.add(stage);
  const frame=new THREE.Mesh(new THREE.BoxGeometry(2.35,2.35,0.25),PAINT.rib);
  frame.position.set(0.6,0,-0.83);
  frame.name='REFERENCE_ONLY_ProbeMountSection';
  stage.add(frame);
  const arms={
    port:makeArm(stage,'port'),
    starboard:makeArm(stage,'starboard'),
  };

  const output=panel();
  const toggles={explode:false,toolOnly:false,orbit:null};
  if(!capture){
    const bar=document.createElement('div');
    bar.style.cssText='position:fixed;top:15px;right:15px;z-index:10;display:flex;gap:8px';
    for(const [label,key] of [['Explode components','explode'],['Isolate tools','toolOnly']]){
      const btn=document.createElement('button');
      btn.type='button';btn.textContent=label;
      btn.style.cssText='padding:8px 11px;border:0;background:#283e49;color:white;cursor:pointer';
      btn.addEventListener('click',()=>{
        toggles[key]=!toggles[key];
        btn.setAttribute('aria-pressed',String(toggles[key]));
      });
      bar.append(btn);
    }
    document.body.append(bar);
    let pointer=null;
    canvas.addEventListener('pointerdown',e=>{
      pointer=e.clientX;canvas.setPointerCapture?.(e.pointerId);
    });
    canvas.addEventListener('pointermove',e=>{
      if(pointer===null)return;
      toggles.orbit=(toggles.orbit??0.5)+(e.clientX-pointer)*0.006;
      pointer=e.clientX;
    });
    for(const event of ['pointerup','pointercancel'])
      canvas.addEventListener(event,()=>{pointer=null;});
  }
  function resize(){
    const w=Math.max(1,canvas.clientWidth||window.innerWidth||1280);
    const h=Math.max(1,canvas.clientHeight||window.innerHeight||720);
    renderer.setSize(w,h,false);
    camera.aspect=w/h;camera.updateProjectionMatrix();
  }
  resize();
  return {
    resize,
    render(t){
      const frame=poseAt(t);
      const exploded=toggles.explode&&!capture?1:frame.explosionFraction;
      for(const side of ['port','starboard']){
        const p=frame.arms[side],a=arms[side];
        a.shoulder.position.set(...p.shoulder);
        setMatrixQuaternion(a.shoulder,p.rotations.shoulder);
        a.elbow.rotation.set(0,p.anglesDegrees.elbow*RAD,0);
        a.wrist.rotation.set(0,p.anglesDegrees.wrist*RAD,0);
        // Visual offset of individual meshes only. Does not modify
        // the FK reference or physics-derived pivot chain.
        a.upper.position.y=exploded*.28;
        a.forearm.position.z=exploded*.25;
        a.tool.position.y=exploded*.35;
        for(const key of ['upper','forearm','tool']){
          a[key].visible=!toggles.toolOnly||key==='tool';
        }
        a.marker.visible=!toggles.toolOnly;
      }
      const az=toggles.orbit!==null&&!capture?toggles.orbit:0.6+t*.035;
      camera.position.set(7.8*Math.cos(az),9*Math.sin(az),5.2);
      camera.lookAt(2.25,0,-1.4);
      const port=frame.arms.port,starboard=frame.arms.starboard;
      const fmt=(v)=>v.map(x=>x.toFixed(2)).join(', ');
      output.textContent=
        'EVERWARD / PRIME GEN-1 — MECHANICAL BRIDGE STUDY\n'+
        'ILLUSTRATIVE ASSET. NOT UE5.8 PRODUCT REALITY.\n'+
        'PORT TOOL TIP m:      ['+fmt(port.toolTip)+']\n'+
        'STARBOARD TOOL TIP m: ['+fmt(starboard.toolTip)+']\n'+
        'Tool contact sphere '+port.toolTipRadiusM.toFixed(2)+' m  |  Exploded '+(exploded*100).toFixed(0)+'%';
      renderer.render(scene,camera);
      return {};
    },
  };
});
