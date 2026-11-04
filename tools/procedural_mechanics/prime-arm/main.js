/**
 * Standalone, exploratory Everward arm engineering film/studio built for the
 * pinned elvarofficial/teardown build system. NOT production game geometry.
 * Build: node .tools/teardown/sims/build.mjs ./tools/procedural_mechanics/prime-arm
 * No imports from Everward's simulation runtime and no network calls.
 */
import * as THREE from 'three';
import {runSim} from 'teardown-kit/runtime.js';
import {DESIGN, poseAt} from './model.mjs';

const SETTINGS = {
  duration: 24,
  fps: 30,
  theme: 'light',
  hud: {enabled: false, title: 'EVERWARD', subtitle: 'MECHANICAL STUDY'},
  director: {enabled: false},
};
const mat = (color, metalness = 0.55, roughness = 0.45) =>
  new THREE.MeshStandardMaterial({color, metalness, roughness});
const shell = mat(0xa2aab1, 0.7);
const inner = mat(0x394852, 0.6);
const brace = mat(0x64757b, 0.75);
const bronze = mat(0xb78343, 0.68);
const caution = mat(0xd89b3a, 0.4);
const black = mat(0x162028, 0.45);
const cylinder = (radius, height, material, segments = 16) => {
  const mesh = new THREE.Mesh(
    new THREE.CylinderGeometry(radius, radius, height, segments),
    material,
  );
  mesh.castShadow = true;
  mesh.receiveShadow = true;
  return mesh;
};
const block = (w, h, d, material) => {
  const mesh = new THREE.Mesh(new THREE.BoxGeometry(w, h, d), material);
  mesh.castShadow = true;
  mesh.receiveShadow = true;
  return mesh;
};

function link(parent, length, radius, label) {
  const group = new THREE.Group();
  group.name = label;
  parent.add(group);
  const shaft = cylinder(radius, length, shell);
  shaft.position.y = -length / 2;
  group.add(shaft);
  for (const sign of [-1, 1]) {
    const rib = block(radius * 0.34, length * 0.9, radius * 0.72, brace);
    rib.position.set(sign * radius * 0.87, -length / 2, 0);
    group.add(rib);
  }
  const conduit = cylinder(radius * 0.13, length * 0.89, black, 8);
  conduit.position.set(0, -length / 2, radius * 1.08);
  group.add(conduit);
  const ferrule = cylinder(radius * 1.28, radius * 0.6, bronze);
  ferrule.position.y = -length + radius * 0.3;
  group.add(ferrule);
  return group;
}
function hinge(parent, label) {
  const joint = new THREE.Group();
  joint.name = label;
  parent.add(joint);
  const core = cylinder(0.225, 0.28, inner, 20);
  core.rotation.x = Math.PI / 2;
  joint.add(core);
  for (const side of [-1, 1]) {
    const bearing = cylinder(0.18, 0.045, bronze, 20);
    bearing.rotation.x = Math.PI / 2;
    bearing.position.z = side * 0.164;
    joint.add(bearing);
  }
  return joint;
}
function readout(container) {
  const panel = document.createElement('div');
  panel.setAttribute('role', 'status');
  panel.style.cssText = [
    'position:fixed', 'left:20px', 'bottom:22px', 'z-index:9',
    'padding:12px 16px', 'background:rgba(16,23,29,.88)',
    'color:#e7ebeb', 'font:13px/1.6 monospace',
    'border:1px solid #6c7c83', 'pointer-events:none', 'white-space:pre',
  ].join(';');
  container.append(panel);
  return panel;
}

runSim(SETTINGS, async ({canvas, capture}) => {
  const scene = new THREE.Scene();
  scene.background = new THREE.Color(0xe8ebeb);
  const camera = new THREE.PerspectiveCamera(38, 1, 0.1, 100);
  const renderer = new THREE.WebGLRenderer({
    canvas, antialias: false, alpha: false, preserveDrawingBuffer: true,
    powerPreference: 'low-power',
  });
  renderer.setPixelRatio(1);
  renderer.outputColorSpace = THREE.SRGBColorSpace;
  scene.add(new THREE.HemisphereLight(0xffffff, 0x667380, 2.1));
  const key = new THREE.DirectionalLight(0xffffff, 3.1);
  key.position.set(3, 5, 6);
  scene.add(key);
  const fill = new THREE.DirectionalLight(0xc8dbe9, 1.2);
  fill.position.set(-5, 0, -4);
  scene.add(fill);

  const stage = new THREE.Group();
  stage.name = 'Everward_Prime_Arm_Exploratory_Study';
  stage.position.set(0, 0.9, 0);
  scene.add(stage);
  const mount = block(0.7, 0.45, 0.58, inner);
  mount.position.y = 0.18;
  stage.add(mount);
  const baseCollar = cylinder(0.35, 0.12, bronze);
  stage.add(baseCollar);

  const shoulder = hinge(stage, 'ARM_SHOULDER');
  const upper = link(shoulder, DESIGN.upperM, DESIGN.upperRadiusM, 'ARM_UPPER_LINK');
  const elbow = hinge(upper, 'ARM_ELBOW');
  elbow.position.y = -DESIGN.upperM;
  const forearm = link(elbow, DESIGN.forearmM, DESIGN.forearmRadiusM, 'ARM_FOREARM_LINK');
  const wrist = hinge(forearm, 'ARM_WRIST');
  wrist.position.y = -DESIGN.forearmM;
  const tool = new THREE.Group();
  tool.name = 'ARM_REPLACEABLE_TOOL_MOUNT';
  wrist.add(tool);
  const toolBody = cylinder(0.125, DESIGN.toolM * 0.67, brace);
  toolBody.position.y = -DESIGN.toolM * 0.33;
  tool.add(toolBody);
  const drill = new THREE.Mesh(
    new THREE.ConeGeometry(0.135, DESIGN.toolM * 0.34, 12),
    black,
  );
  drill.position.y = -DESIGN.toolM * 0.83;
  drill.rotation.z = Math.PI;
  drill.name = 'ARM_SAMPLE_DRILL';
  tool.add(drill);
  const beacon = cylinder(0.16, 0.04, caution);
  beacon.position.y = -0.07;
  tool.add(beacon);

  const panel = readout(canvas.parentElement || document.body);
  const controls = document.createElement('div');
  controls.style.cssText = [
    'position:fixed', 'top:18px', 'right:20px', 'z-index:10',
    'display:flex', 'gap:8px',
  ].join(';');
  const modes = {forceExplode: false, forceIsolate: false, azimuth: null};
  for (const [name, flag] of [['Explode parts', 'forceExplode'], ['Isolate tool', 'forceIsolate']]) {
    const button = document.createElement('button');
    button.type = 'button';
    button.textContent = name;
    button.style.cssText = 'padding:8px 11px;background:#27343b;color:white;border:0;cursor:pointer;font:12px monospace';
    button.addEventListener('click', () => {
      if (!capture) {
        modes[flag] = !modes[flag];
        button.setAttribute('aria-pressed', String(modes[flag]));
      }
    });
    if (capture) button.hidden = true;
    controls.append(button);
  }
  document.body.append(controls);

  if (!capture) {
    let lastX = null;
    canvas.addEventListener('pointerdown', (e) => {
      lastX = e.clientX;
      canvas.setPointerCapture?.(e.pointerId);
    });
    canvas.addEventListener('pointermove', (e) => {
      if (lastX === null) return;
      modes.azimuth = (modes.azimuth ?? 0.45) + (e.clientX - lastX) * 0.008;
      lastX = e.clientX;
    });
    canvas.addEventListener('pointerup', () => { lastX = null; });
    canvas.addEventListener('pointercancel', () => { lastX = null; });
  }

  function resize() {
    const w = Math.max(1, canvas.clientWidth || window.innerWidth || 1280);
    const h = Math.max(1, canvas.clientHeight || window.innerHeight || 720);
    renderer.setSize(w, h, false);
    camera.aspect = w / h;
    camera.updateProjectionMatrix();
  }
  resize();
  return {
    resize,
    render(t) {
      const p = poseAt(t);
      const exploded = modes.forceExplode && !capture ? 1 : p.explosionFraction;
      shoulder.rotation.z = p.angles.shoulderRad;
      elbow.rotation.z = p.angles.elbowRad;
      wrist.rotation.z = p.angles.wristRad;
      elbow.position.x = exploded * 0.36;
      wrist.position.z = exploded * 0.32;
      tool.visible = !(modes.forceIsolate && !capture);
      upper.visible = !(modes.forceIsolate && !capture);
      forearm.visible = !(modes.forceIsolate && !capture);
      drill.rotation.y = t * 5;
      const angle = modes.azimuth !== null && !capture ? modes.azimuth : 0.55 + 0.5 * Math.sin(t * 0.11);
      camera.position.set(6.4 * Math.sin(angle), -0.63, 6.4 * Math.cos(angle));
      camera.lookAt(0, -0.85, 0);
      panel.textContent =
        'EVERWARD / PRIME ARM — EXPERIMENTAL ENGINEERING STUDY\n' +
        'NOT CANONICAL DIMENSIONS / NOT GAMEPLAY PHYSICS\n' +
        'Shoulder ' + p.angles.shoulderRad.toFixed(3) + ' rad  Elbow ' +
        p.angles.elbowRad.toFixed(3) + ' rad  Wrist ' +
        p.angles.wristRad.toFixed(3) + ' rad\n' +
        'Nominal tip (' + p.tip.x.toFixed(3) + ', ' + p.tip.y.toFixed(3) +
        ') m   Exploded ' + (exploded * 100).toFixed(0) + '%';
      renderer.render(scene, camera);
      return {};
    },
  };
});
