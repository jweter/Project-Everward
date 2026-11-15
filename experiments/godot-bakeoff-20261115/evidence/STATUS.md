# Everward Godot 4.7.2 — Initial proof, 2026-11-15

**Candidate:** Godot 4.7.2, GL Compatibility, Windows 11, Intel Iris Xe, 15.72 GiB RAM.

**Isolation:** No modification to Unreal production repo or Iron God. This uses provisional GDScript mechanics; authoritative C++ simulation NOT yet bound.

## Measured
- Headless mechanics self-test: exit 0, 8/8 assertions PASS.
- Windowed 1280 x 720 rendering smoke: exit 0. 240 application frames, first 60 excluded.
- ONE trial ONLY: approximate mean 32.05 FPS from 180 frame-loop deltas; p95 frame delta 31.80 ms; p99 38.16 ms.
- Windowed process wall time including startup/shutdown: 10.86 sec.
- Screenshots visual_070.png (damaged) and visual_140.png (repaired), both save result 0.
- GPU utilization, system RAM, CPU utilization, cold compile, package size, subjective play and matched Unreal not measured.
- Rendering uses crude primitives; this is not a visual-fidelity comparison.

## Not yet complete
1. Bind actual Everward C++ simulation; verify identical seed and save.
2. Match canonical art, camera and controls with Unreal.
3. Measure system/process RAM, CPU/GPU, three repeated trials and representative frame pacing.
4. Package standalone Windows builds and run unattended gameplay-input acceptance.
5. Run matched Unreal and Iron God local-builder tests.
6. Conduct owner enjoyment review only after objective checks.

Decision: PENDING — Unreal remains accepted production engine.

Play by double-clicking the adjacent Play Godot Prototype.bat.
Controls: WASD/QE navigate; T scan; Y auto-approach; F mine; R repair; F5 save; F6 load; H HUD toggle.
