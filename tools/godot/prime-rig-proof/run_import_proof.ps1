# Dependency-free export + import proof. Does not mutate Unreal, production files or Iron God.
param(
    [Parameter(Mandatory = $true)][string]$GodotExe,
    [string]$WorkDir = ""
)
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\..'))
if (-not (Test-Path -LiteralPath $GodotExe -PathType Leaf)) { throw "Godot executable missing: $GodotExe" }
if (-not (Get-Command node -ErrorAction SilentlyContinue)) { throw 'Node.js 22+ required to export existing Everward GLB' }
if (-not $WorkDir) {
    $WorkDir = Join-Path $env:TEMP ('everward-prime-rig-' + [guid]::NewGuid().ToString('N'))
}
$WorkDir = [IO.Path]::GetFullPath($WorkDir)
if ($WorkDir.StartsWith($repo + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase) -or $WorkDir.Equals($repo, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Proof output must be outside the source repository'
}
$assets = Join-Path $WorkDir 'assets'
New-Item -ItemType Directory -Force -Path $assets | Out-Null
Copy-Item (Join-Path $PSScriptRoot 'project.godot') (Join-Path $WorkDir 'project.godot')
Copy-Item (Join-Path $PSScriptRoot 'verify_import.gd') (Join-Path $WorkDir 'verify_import.gd')
Copy-Item (Join-Path $PSScriptRoot 'physics_smoke.gd') (Join-Path $WorkDir 'physics_smoke.gd')
$arm = Join-Path $repo 'tools\procedural_mechanics\prime-arm'
Write-Output 'STEP: authoritative geometry/export tests (Windows C++ via CMake/MSVC fallback)'
& node --test (Join-Path $arm 'model.test.mjs') (Join-Path $arm 'export_glb.test.mjs')
if ($LASTEXITCODE -ne 0) { throw 'Arm geometry/export tests failed' }
# Godot auto-imports OBJ files; keep MSVC .obj intermediates OUTSIDE the Godot project.
$build = $WorkDir + '-oracle_build'
if ($IsWindows -or $env:OS -eq 'Windows_NT') {
    & cmake -S $PSScriptRoot -B $build -G 'Visual Studio 18 2026' -A x64
    if ($LASTEXITCODE -ne 0) { throw 'MSVC C++ oracle configure failed' }
    & cmake --build $build --config Release --parallel 2
    if ($LASTEXITCODE -ne 0) { throw 'MSVC C++ oracle build failed' }
    $oracle = Join-Path $build 'Release\everward_prime_arm_oracle.exe'
} else {
    & cmake -S $PSScriptRoot -B $build
    if ($LASTEXITCODE -ne 0) { throw 'C++ oracle configure failed' }
    & cmake --build $build --parallel 2
    if ($LASTEXITCODE -ne 0) { throw 'C++ oracle build failed' }
    $oracle = Join-Path $build 'everward_prime_arm_oracle'
}
& node (Join-Path $PSScriptRoot 'verify_cpp_bridge.mjs') $oracle
if ($LASTEXITCODE -ne 0) { throw 'C++ / exported geometry parity failed' }
Write-Output 'STEP: deterministic glTF export'
& node (Join-Path $arm 'export_glb.mjs') $assets
if ($LASTEXITCODE -ne 0) { throw 'Arm glTF export failed' }
Write-Output 'STEP: native Godot import in isolated temporary project'
& $GodotExe --headless --editor --import --path $WorkDir
if ($LASTEXITCODE -ne 0) { throw 'Godot glTF scene import failed' }
Write-Output 'STEP: verify imported named meshes and authoritative pivot/tool-tip markers'
& $GodotExe --headless --path $WorkDir --script 'res://verify_import.gd'
if ($LASTEXITCODE -ne 0) { throw 'Godot imported rig verification failed' }
& $GodotExe --headless --path $WorkDir --script 'res://physics_smoke.gd'
if ($LASTEXITCODE -ne 0) { throw 'Godot Jolt local collision/query smoke failed' }
$sha = (git -C $repo rev-parse HEAD).Trim()
$report = [ordered]@{
    schema_version = 1
    status = 'PASS'
    source_sha = $sha
    engine = 'Godot (actual binary supplied to runner)'
    implementation = 'EXPERIMENTAL_GLTF_IMPORT_PROOF_ONLY'
    verified = @('source-vs-articulated-arm math 40 poses / 360 axes', 'Godot GLB import and named pivots', 'Jolt local CCD/drag/collision-layer/query smoke')
    not_verified = @('canonical full Prime A mesh', 'gameplay', 'realism', 'Unreal render parity', 'GPU frame times', 'Godot physics determinism')
    artifacts = @('assets/prime-arm-assembly.glb','assets/prime-arm-modules.glb','assets/prime-arm-manifest.json')
}
$report | ConvertTo-Json -Depth 5 | Set-Content -Encoding UTF8 (Join-Path $WorkDir 'proof_receipt.json')
Write-Output ('EVERWARD_PRIME_RIG_PROOF_STATUS=PASS WORKDIR=' + $WorkDir)
