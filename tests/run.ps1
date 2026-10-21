param([string[]]$Suites = @('keyboard_read', 'cd_audio', 'far_depth', 'alpha_pixels', 'transition_gate', 'presentation', 'widescreen', 'orbit_camera', 'coop_camera', 'coop_runtime', 'first_person', 'first_person_runtime', 'first_person_props', 'first_person_floor', 'camera_runtime', 'mouse_combat', 'camera_collision', 'modern_input', 'modern_runtime', 'analog_movement', 'analog_runtime'))
$ErrorActionPreference = 'Stop'
$ffRoot = Split-Path $PSScriptRoot -Parent
. (Join-Path $ffRoot 'scripts\devshell.ps1')
New-Item -ItemType Directory -Path (Join-Path $ffRoot 'build') -Force | Out-Null
Push-Location (Join-Path $ffRoot 'build')
try {
    foreach ($ffSuite in $Suites) {
        if ($ffSuite -notmatch '^[a-z_]+$') { throw 'Invalid test suite name' }
        $ffSource = Join-Path $PSScriptRoot ($ffSuite + '_test.cpp')
        if (-not (Test-Path -LiteralPath $ffSource)) { throw ('Test source missing: ' + $ffSuite) }
        & cl.exe /nologo /MT /EHsc /W4 $ffSource ('/Fe:' + $ffSuite + '_test.exe') /link user32.lib dxguid.lib dinput8.lib
        if ($LASTEXITCODE -ne 0) { throw ('Test compilation failed: ' + $ffSuite) }
        & (Join-Path '.' ($ffSuite + '_test.exe'))
        if ($LASTEXITCODE -ne 0) { throw ('Test failed: ' + $ffSuite) }
    }
} finally { Pop-Location }
