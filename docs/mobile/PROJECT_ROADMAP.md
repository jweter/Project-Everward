# Everward Mobile — New Mobile Project Charter

**Planning status:** proposed separate iOS product track; **not** a current executable iOS app, installed service, or owner runtime-mode change. 2026-11-01.

**Parent project:** [Everward](https://github.com/jweter/Project-Everward)  
**Product form:** Native, fully offline Unreal space exploration game for iPhone  
**Project ID:** `everward_mobile`

## Vision and product promise

Build a truly standalone iPhone adaptation of Everward's One Probe journey, running its authoritative engine-independent simulation locally on-device and presenting a touch-first Unreal experience. It must be a meaningful self-contained space game, not a remote stream of the Windows version or a separate divergent simulation.

## Current verified foundation

Everward's accepted production direction is Unreal Engine 5.8. It has an engine-independent C++20 simulation under `src/simulation/` and an Unreal `UProbeSimulationAdapter`, GameMode/Pawn/HUD, fixed-step simulation and initial One Probe environment. First playable is still under development; not yet a proven iOS product or game build.

## First real user journey

In airplane mode, launch the game, enter one star-system scene as the probe, move and scan, observe power/heat/storage, collect one resource through the authoritative simulation, use it for one meaningful progression/construction step, save/quit/relaunch, and finish the repeatable One Probe objective without an account or backend.

## Non-negotiable standalone contract

- Unreal iOS build includes authoritative C++ simulation and local deterministic save format; no simulation service or remote streaming.
- One star system, one probe, readable touch controls, scanner, power management and one resource/progression loop.
- Graphics/performance tiers with low-end defaults; opt-in higher quality based on actual device thermal/battery measurements.
- No multiplayer, cross-device synchronization, online leaderboards or paid service required for first playable mobile game.

## Optional connected expansion (not a dependency)

Optional future cloud saves or cross-device handoff can be scoped after the standalone game is stable. Do not impose account or always-online DRM for core gameplay. Mac/Xcode iOS build infrastructure is a development-time requirement, not a gameplay-time server.

## Out of scope and honest feasibility boundary

No App Store or playable native Unreal C++ iOS release can be claimed until Mac/Xcode signing and real device build are available. All player gameplay after installation can remain fully offline.

## Portfolio operating principles

- Phone-first: app launches and performs its declared MVP value in airplane mode without Azathoth, GitHub, a hosted API, or a paid service.
- Native-first for the long run; a PWA or companion prototype may validate UX sooner, but may not be represented as proof of fully native offline functionality.
- One source of product truth: share formats, algorithms and fixtures with parent project; never fork the entire Python/Windows backend or invent duplicate business rules.
- Optional sync must be explicit, authenticated, revocable and resilient to disconnection; an offline session must never trigger an implicit network request.
- Protect user data: use iOS sandbox, Files integration, file protection/Keychain for secrets; avoid collecting unnecessary user content and telemetry.
- Keep parent project quality policies: narrow branches, exact-head tests, licensing/provenance checks, review and product reality. Do not install or alter the running Iron God release.
- Measure device performance, memory, thermals and battery on physical supported iPhones; record actual evidence before making capability or compatibility claims.
- Maintain accessibility (VoiceOver, Dynamic Type where applicable, contrast, touch targets), cancel/resume behavior and storage bounds.
- No Apple membership purchase, Mac rental, new paid service, signing certificate or public App Store distribution without separate explicit owner decision.

## Build environment and cost constraints

Native iOS signing, on-device testing and App Store submission normally require macOS/Xcode infrastructure; the Windows laptop can remain the source-development and test workstation. Start with portable C++/Swift contract fixtures and SwiftUI design prototypes where a Mac runner/device is not yet available. Personal device development can start without paid Apple Developer Program membership, subject to Apple's provisioning limits; App Store/TestFlight distribution requires program enrollment.

Default cost target $0 before owner-approved developer membership, build-machine access or cloud usage. Keep optional model downloads, paid APIs, hosted inference and other recurring costs opt-in. No automatic spending.

## Parent-code and platform references

- https://github.com/jweter/Project-Everward/blob/main/unreal/README.md
- https://github.com/jweter/Project-Everward/blob/main/docs/ROADMAP.md
- https://dev.epicgames.com/documentation/unreal-engine/ios-ipados-and-tvos-development-requirements-for-unreal-engine


---

# Everward Mobile — Phased iPhone Roadmap

**Status:** PLANNED. Phases are evidence gates, **not calendar commitments**. Parent projects retain authority over their existing implementation and product quality.

## Release strategy

Start by proving a no-server/no-laptop local user journey. Give native/local capabilities their own implementation and acceptance tests. Add external integrations only after the standalone core works. For every phase: maintain a narrow branch, exact-head tests, real device evidence and explicit limitations; do not call draft prototypes complete.

## P0 — Build/toolchain and shared-simulation feasibility

**Deliverables**

- Confirm compatible Mac + Xcode + Unreal 5.8 signed iOS build path (or explicitly document build blocker).
- Compile and run the engine-independent C++ simulation tests for iOS target; prove cross-platform deterministic snapshots.
- Build a minimal iOS Unreal scene from existing probe pawn/adapter; record exact device and package/memory/fps evidence.

**Exit gate:** A signed physical-iPhone smoke build runs the real C++ probe simulation, or mobile implementation remains gated without pretending success.

## P1 — Offline One Probe control slice

**Deliverables**

- Port visible probe, fixed-step simulation, camera and touch controls with no mechanical rules duplicated in UI.
- Expose scan target and deterministic HUD power/thermal/velocity; initial scene should load offline.
- Add on-device local save/restore and permission-free first launch.

**Exit gate:** Move and scan in a single offline scene, save and reload exact authoritative probe state on device.

## P2 — One playable loop

**Deliverables**

- Add resource gathering using authoritative commands, a real storage consequence and one construction/repair/progression use.
- Connect UI feedback for success/failure, energy/heat constraints and a repeatable objective.
- Prove same deterministic outcome from identical inputs in headless C++ and iPhone Unreal runtime.

**Exit gate:** A real user completes observe→scan→gather→build/repair→repeat without network, cheating core simulation, or relying on desktop assets not shipped.

## P3 — iPhone performance and UX

**Deliverables**

- Profile representative low/mid/high target devices; set measured device tiers for frame pacing, memory, battery and thermal state.
- Replace unsuitable desktop visuals/shaders/assets with scalable mobile LOD and texture budgets while preserving art identity.
- Add touch accessibility, optional controller and interruption/resume tests.

**Exit gate:** Sustained playtest meets an explicitly measured baseline tier and survives thermal load with readable controls, no severe crashes or save corruption.

## P4 — Game beta and native distribution

**Deliverables**

- Run extended offline progression, storage/save migration and crash recovery tests on physical devices.
- Review Unreal/Epic asset and third-party IP licensing, app size, privacy and release platform policies.
- Prepare TestFlight/App Store only after approved Apple build/distribution cost and Mac availability.

**Exit gate:** One Probe mobile game demonstrates authentic Product Reality and is playable entirely offline after installation.

## P5 — Long-range parity (deferred)

**Deliverables**

- Only after iPhone first playable: more systems, richer procedural worlds and late-game mechanics, evaluated against mobile runtime constraints.
- Keep desktop gameplay milestones authoritative; use mobile tiers rather than fork mechanics.
- Defer cross-save and network services until owner chooses them.

**Exit gate:** Expansion is justified by live player/device evidence rather than parallel speculative features.

## First independently actionable engineering slices

1. EVM-001: confirm Unreal 5.8/Xcode/iPhone iOS build setup and cost-neutral path
2. EVM-002: compile and compare headless simulation snapshots on iOS
3. EVM-003: one-probe Unreal iPhone rendering and touch-control spike
4. EVM-004: offline save/reload deterministic acceptance
5. EVM-005: device performance tier evidence and One Probe mini-loop

## External blockers / decisions held for later

No App Store or playable native Unreal C++ iOS release can be claimed until Mac/Xcode signing and real device build are available. All player gameplay after installation can remain fully offline.

Open decisions to defer: name/icon/branding; minimum iOS version and target devices after a build smoke; distribution model and monetization; Apple signing infrastructure; optional online sync; optional on-device inference models. None block offline planning and portable-contract work.

## Change-control and roadmap hygiene

This iOS roadmap is a distinct **proposal** and does not silently expand the owner-approved desktop implementation roadmap. On approval of a phase, add narrowly bounded capability graph entries or planning issues in the appropriate development project; do not auto-enroll these proposals in the active Iron God production dispatcher, grant runtime execution rights, or compete with its owner/session control. Cross-repo dependencies must reference the specific parent schema/commit and explicit acceptance evidence. Record verified milestone outcomes, not guesses.


---

# Everward Mobile — Architecture, Risks and Validation

## System components and reuse seams

- `SimulationCore` and `ProbeRuntime`: reuse existing deterministic engine-independent C++ code; add iOS platform serialization tests without changing mechanical truth.
- `MobilePresentation`: Unreal renderer and scalable assets with mobile graphics profiles; `UProbeSimulationAdapter` remains the only authoritative simulation bridge.
- `TouchInput`: thumb controls/gesture camera, large tap targets, controller support optional; `MobileHUD`: collapsed capability-driven systems panel.
- `LocalSave`: versioned deterministic checkpoint with integrity and migration; `PerfHarness`: fps/thermal/memory telemetry captured on real iPhones.

## Offline / optional-online data flow

**Offline:** Files / mic / touch input → local validated parser → versioned local store → deterministic logic and (if proven) on-device inference → local UI / Files export. No remote call is necessary for the core workflow.

**Online enhancement:** explicit user action → permission/identity check → versioned optional adapter → external backend or online public source → source/receipt validation → local persisted result. Network failure leaves the offline product functional; remote-only operations fail visibly and never impersonate local results.

## Technical and delivery risks

- **iOS toolchain:** Signed C++ Unreal iOS builds need compatible Mac/Xcode; no free Windows-only production signing path assumed.
- **Rendering and thermal:** Unreal desktop features may be too expensive; mobile tier and physical-device profiling required.
- **Development diversion:** Mainline desktop One Probe must remain the primary shared simulation truth; mobile work starts with narrow experimental branch/spike.
- **Save compatibility:** Cross-platform C++ struct layout/floating-point/time-step differences require schema and deterministic regression fixtures.

## Cross-cutting real-device acceptance matrix

1. Fresh install + airplane mode + Azathoth completely off: app launches and completes the documented core user journey without stalls or hidden remote services.
2. Cold start, memory peak, battery/thermal behavior, model/download size and task latency measured on an actual physical device. Set release budgets from the first spike rather than inventing benchmark successes.
3. No unauthenticated inbound listening port, leaked token, bundled source secret, plaintext private sync credential or surprise upload; perform negative network test.
4. App returns a visible failure state when inputs, permissions, local model assets or Files permissions are missing. No fabricated output or false completion.
5. User can export, back up and delete locally created work without a server; app survives force quit and offline relaunch without losing completed edits.
6. Cross-platform contract/fixture tests verify imported/exported formats against the parent repository; snapshots are not treated as live or authoritative.
7. Standard automated checks, independent review, privacy/permissions declarations, App Store licensing/provenance review and actual iPhone product test must pass before external beta.

## Release evidence template

- Exact source head and build identity; Xcode/SDK/device/OS and model/runtime version if used.
- Recorded airplane-mode test with desktop/server powered off, input fixtures, expected outputs, pass/fail and timings.
- Memory/CPU/battery/thermal/storage measurements; accessible UI screen review; interruption/resume evidence.
- Data privacy/security review, license inventory, signed-build provenance, reviewer approval and open blocker list.
- Explicit distinction: proposed concept vs prototype vs device-proven vs App Store distributable.

## References

- https://github.com/jweter/Project-Everward/blob/main/unreal/README.md
- https://github.com/jweter/Project-Everward/blob/main/docs/ROADMAP.md
- https://dev.epicgames.com/documentation/unreal-engine/ios-ipados-and-tvos-development-requirements-for-unreal-engine
