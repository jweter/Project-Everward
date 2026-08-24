# Generation-1 Prime Probe — Modeling Checklist

## Reference-package preflight

Before starting or reviewing derived work, run:

```bash
python tools/validate_reference_assets.py
```

This verifies every canonical/exploratory source image against the versioned
SHA-256 and pixel dimensions. A mismatch is provenance drift and must be
resolved explicitly; do not silently replace a canonical source.

## Blockout gate

- overall length and width match the canonical scale sheet closely enough for gameplay camera evaluation;
- central structural spine and major cylindrical/structural masses are represented;
- primary propulsion axis and engine envelope are correct;
- radiator geometry and deployment envelope are represented;
- primary manipulator arms have correct approximate pivot locations and reach;
- major sensor/antenna assemblies are placed;
- no cockpit, human windows, aerodynamic wings, or fighter-like silhouette are introduced;
- camera can orbit the probe without clipping through the major silhouette.

## Articulation gate

- manipulator shoulder/elbow/wrist joints are independently riggable;
- radiator deployment, if animated, has explicit pivots and safe clearances;
- tool/end-effector attachment interfaces are modular;
- sensor booms/antennae that need deployment have explicit pivots;
- thruster and engine exhaust directions are represented by named sockets/locators.

## Production-mesh gate

- panel breaks, fasteners, structural joints, cable runs, and access/service regions follow the reference language;
- geometry distinguishes structural metal, protective panels, ceramic/thermal areas, composite members, radiator surfaces, optics, engine hardware, and robotic joints;
- material IDs are assigned by physically meaningful material family, not by arbitrary visual color;
- no high-frequency detail is modeled that should instead be normal/decal/texture detail unless silhouette or animation requires it.

## Unreal integration gate

- canonical dimensions are verified after import;
- pivot/origin is documented and stable;
- collision is intentional rather than auto-generated blindly;
- named sockets exist for engines, RCS, sensors, manipulators/tools, camera reference, and future VFX/SFX attachment points;
- material slots correspond to the approved material families;
- blockout and production assets use replaceable interfaces so simulation/gameplay code is not coupled to mesh internals;
- visual comparison screenshots are captured against the canonical reference set before approval.
