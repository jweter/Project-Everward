/**
 * Original Everward procedural hard-surface module vocabulary, normalized to
 * unit-length mesh components. Local pivot = module geometric centre.
 * Geometry is appearance-only; never replaces ManipulatorRig/contact.
 *
 * Upper and forearm longitudinal axes are +X; toolhead longitudinal axis +Z
 * to match the Unreal component's existing 90-degree Pitch rotation.
 * Current pawn component transforms therefore remain exactly unchanged.
 * Shapes are deliberately inexpensive for 16 GB/integrated-GPU testing.
 */
export const PALETTE = Object.freeze({
  alloy: {baseColor:[0.31, 0.36, 0.39, 1], metallic:0.75, roughness:0.44},
  rib: {baseColor:[0.12, 0.17, 0.20, 1], metallic:0.65, roughness:0.55},
  trim: {baseColor:[0.65, 0.41, 0.18, 1], metallic:0.7, roughness:0.48},
  tool: {baseColor:[0.17, 0.14, 0.12, 1], metallic:0.74, roughness:0.52},
});

export const MODULES = Object.freeze({
  upper: {
    name:'EW_Arm_Upper',
    axis:'X',
    parts:[
      {kind:'box', size:[0.91,0.60,0.66], center:[0,0,0], material:'alloy'},
      {kind:'box', size:[0.92,0.20,0.90], center:[0,0.34,0], material:'rib'},
      {kind:'box', size:[0.92,0.20,0.90], center:[0,-0.34,0], material:'rib'},
      {kind:'box', size:[0.15,0.85,0.93], center:[-0.39,0,0], material:'trim'},
      {kind:'box', size:[0.12,0.85,0.93], center:[0.40,0,0], material:'trim'},
      {kind:'box', size:[0.72,0.13,0.15], center:[0,0,0.43], material:'tool'},
    ],
  },
  forearm: {
    name:'EW_Arm_Forearm',
    axis:'X',
    parts:[
      {kind:'box', size:[0.94,0.53,0.55], center:[0,0,0], material:'alloy'},
      {kind:'box', size:[0.98,0.15,0.79], center:[0,0.30,0], material:'rib'},
      {kind:'box', size:[0.98,0.15,0.79], center:[0,-0.30,0], material:'rib'},
      {kind:'box', size:[0.12,0.70,0.75], center:[-0.39,0,0], material:'trim'},
      {kind:'box', size:[0.12,0.70,0.75], center:[0.39,0,0], material:'trim'},
      {kind:'box', size:[0.72,0.10,0.15], center:[0,0,0.38], material:'tool'},
    ],
  },
  tool: {
    name:'EW_Arm_MiningTool',
    axis:'Z',
    parts:[
      {kind:'cylinder', bottom:0.34, top:0.34, height:0.53,
        center:[0,0,-0.16], segments:12, material:'tool'},
      {kind:'cylinder', bottom:0.44, top:0.44, height:0.10,
        center:[0,0,-0.35], segments:12, material:'trim'},
      {kind:'cylinder', bottom:0.25, top:0.31, height:0.12,
        center:[0,0,0.18], segments:12, material:'alloy'},
      {kind:'cylinder', bottom:0.31, top:0.02, height:0.34,
        center:[0,0,0.32], segments:12, material:'tool'},
    ],
  },
});
