# Phase 2 Scan-to-Mining + Manipulator Selection Test

Status: **Implemented; Product Reality pending in the local Unreal build.**

This slice turns the existing scanner and articulated mining arms into one small gameplay loop:

**survey -> identify resource -> approach -> deploy/aim tool -> mine -> accumulate material**

It also makes manipulator selection visible on the physical probe. Selecting a different arm or joint in the manipulator HUD must change the highlighted hardware on the machine itself.

## Expected startup state

- The manipulator control panel is visible by default.
- Port shoulder is the initial selection.
- The selected Port shoulder/upper-arm segment is visibly highlighted.
- The Phase-2 resource body reads as **UNSURVEYED RESOURCE BODY**.

## Scanner purpose test

1. Open/select **Sensors** in the systems panel.
2. Ensure sensors have at least the Generation-1 minimum operating power.
3. Press **Enter** to scan `SCAN-001`.
4. Let the scan complete.
5. Confirm the resource-body readout changes from **UNSURVEYED** to:
   - `Iron-bearing silicate regolith`
   - remaining extractable kilograms
   - the `[G] MINE` instruction.

**Pass:** completing the scan changes what the player knows and unlocks mining eligibility. Mining before scanning must be rejected.

## Manipulator-selection visibility test

With the manipulator panel open:

1. Press **N** to switch Port/Starboard selection.
2. Confirm the highlight switches to the selected physical arm.
3. Press **4** (Shoulder). Confirm shoulder + upper-arm hardware highlights.
4. Press **5** (Elbow). Confirm the selected arm's forearm highlights.
5. Press **6** (Wrist). Confirm the selected arm's mining tool head highlights.
6. Press **M** to close the manipulator panel. Confirm the selection highlight clears.

**Pass:** without reading the HUD text, the player can tell which arm segment the next joint command will move.

## Mining test

1. Complete the scanner test above.
2. Approach the resource body slowly. Use **Space** to brake before impact.
3. Deploy either arm with **1** or **2**.
4. Attach the mining tool with **3**.
5. Use **N**, **4/5/6**, and **, / .** to position the selected tool near the surface.
6. Press **G**.

Expected rejection messages include:

- scan target first;
- deploy a manipulator arm and attach its tool;
- mining tool is not in reach;
- storage is full;
- deposit exhausted.

When all requirements are met, pressing **G** extracts one Generation-1 mining cycle (currently 5 kg, capped by remaining deposit and free storage). The resource-body readout must decrease its remaining mass and the high-visibility notice must report the recovered mass.

## Safety/physics expectations

- Mining must not work remotely from arbitrary distance.
- The arm/environment collision guard must continue preventing impossible penetration into the body.
- The probe hull/contact solver remains authoritative for probe-body collision.
- Mining does not replace or bypass existing impact damage, scan, power, or manipulator state.

## Product Reality evidence to capture

Capture screenshots showing:

1. unsurveyed target + highlighted shoulder;
2. completed scan + identified material and remaining mass;
3. elbow selection highlight;
4. wrist/tool selection highlight at the resource surface;
5. successful mining notice with reduced deposit mass;
6. at least one rejected out-of-reach mining attempt.

If these pass in the local Unreal build, this is the first direct **scanner -> manipulator -> resource acquisition** gameplay chain in Everward.
