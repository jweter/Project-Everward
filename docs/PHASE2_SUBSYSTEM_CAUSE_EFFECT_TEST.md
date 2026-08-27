# Phase 2 — Subsystem Cause/Effect Product Reality Test

Status: implementation branch; local Unreal Engine 5.8 Product Reality required before Slice 2 is complete.

## Purpose

This pass turns the compact subsystem HUD from a status list into an operating explanation layer. The player should be able to see not only that a capability is `READY`, `LOCKED`, or `FAILED`, but also why, what power threshold matters, what an attempted command did, and what the Generation-1 automation changed.

The implementation remains deliberately small. It uses mechanics that already exist and adds one new authoritative Generation-1 sensor-power floor. It does not pretend the final propulsion-power, active thermal-control, component-damage, or repair models are solved.

## Expected starting state

Canonical EV-0001 begins with:

- Sensors: **50 W** allocated, **50 W minimum**, `READY`;
- Computation: **25 W** allocated, **25 W minimum for automation**, `READY`;
- Propulsion: current Phase-2 command-driven model;
- Thermal Control: current passive-cooling model;
- total starting allocation: **75 W**;
- passive generation: **75 W**.

That makes the initial idle configuration energy-neutral while keeping scanning and the primitive policy executor available.

## Local UE 5.8 test script

Run the exact CI-green build and test in one session.

### A. At-a-glance explanations

1. Start the build without changing power.
2. Confirm the collapsed systems panel shows all installed systems simultaneously.
3. Confirm each row includes live watts, `READY` / `LOCKED` / `FAILED`, and a short reason beneath it.
4. Open `Tab` details and confirm the selected subsystem repeats the reason and any minimum operating-power requirement.

Pass condition: you should not have to infer why a system is locked from unrelated numbers elsewhere on the HUD.

### B. Sensor power has a real consequence

1. Select Sensors.
2. Confirm Sensors begin at 50 W and scanning works with `Enter`.
3. While a scan is active, press `Page Down` once. Sensors should drop to 25 W.
4. Confirm the active scan aborts rather than completing for free.
5. Confirm Sensors now show `LOCKED` with `BELOW MINIMUM POWER // NEED 50 W`.
6. Press `Enter` and confirm the new scan is rejected with a brief global message explaining the 50 W minimum.
7. Press `Page Up` once to restore Sensors to 50 W.
8. Confirm Sensors return to `READY`.
9. Start a new scan and confirm it progresses normally.

Pass condition: changing sensor power must visibly change what the probe can do, and the failure/recovery path must be understandable without guessing.

### C. Automation explains what it changed

1. Clear any active scan and leave Sensors at 50 W.
2. Select Computation.
3. Confirm Computation has at least 25 W and the executor says `RUNNING`.
4. Press `Enter` to install `GEN1 BASIC SURVIVAL`.
5. EV-0001 currently begins below the policy's deliberately aggressive 60% energy threshold, so the first evaluation should shed Sensors from 50 W to 0 W.
6. Confirm a visible automation notice states approximately:

   `AUTOMATION: Sensors 50 W -> 0 W // energy reserve below 60%`

7. Confirm the Sensors row simultaneously changes to `LOCKED` because it is now below its 50 W minimum.
8. Open Computation details and confirm the most recent automation action remains inspectable there after the brief global banner disappears.
9. Manually restore Sensors while the policy remains installed and confirm the primitive policy sheds them again on its next evaluation.
10. Clear the policy, restore Sensors to 50 W, and confirm they remain available.

Pass condition: automation should feel like the probe actually executed a rule against shared state, not like an invisible script changed an unrelated UI value.

### D. Rejected commands are obvious

1. With Sensors below 50 W, attempt a scan.
2. Confirm `COMMAND REJECTED // ...` appears outside the expanded systems panel for a few seconds.
3. Confirm the expanded panel still preserves the last command detail for deeper inspection.

Pass condition: an input that cannot execute must never look like a dead key.

### E. No false discoveries

1. Start a scan with Sensors at 50 W.
2. Reduce Sensors below 50 W before it completes.
3. Confirm the scan aborts.
4. Confirm **no** `SCAN COMPLETE` / discovery result is created from that aborted scan.
5. Restore Sensors and complete a fresh scan.
6. Confirm the real completion still produces the persistent discovery result.

This specifically verifies the new authoritative scan lifecycle projection. Completion/cancellation truth now comes from simulation domain events rather than being guessed from the most recent manual command.

## Product Reality questions

Record these after the run:

- Can you tell why a subsystem is unavailable within one glance?
- Does the 50 W sensor threshold make power management feel more like operating a machine?
- Does aborting a scan on sensor under-power feel reasonable, or should later generations pause/resume instead?
- Is four seconds long enough for a rejected-command banner?
- Is six seconds long enough for an automation-action banner?
- Does the automation message make the cause/effect obvious?
- Does the compact panel remain readable with the new reason line under each subsystem?
- Does any status reason overlap, clip, or become difficult to read at the current resolution?

## Gate

This PR may be CI-green and mergeable before local evidence under the repository's parallel-safe lane, but Slice 2 is not complete until Product Reality confirms the behavior. A local failure in the already-merged orientation/righting pass still outranks later roadmap work and must be repaired first.