param([string[]]$Suites = @('item_pickup_overlay', 'item_pickup', 'item_pickup_runtime', 'throw_trajectory', 'throw_trajectory_runtime', 'combat_focus', 'combat_focus_runtime', 'weapon_laser_runtime', 'campaign_save', 'campaign_slots', 'campaign_menu', 'campaign_pause_runtime', 'campaign_runtime', 'weapon_feedback_runtime', 'prop_feedback_runtime', 'input_activity', 'input_hints_runtime', 'controller_feedback', 'combat_feedback_runtime', 'guard_animation', 'player_guard', 'player_guard_runtime', 'source_runtime', 'control_settings', 'control_menu_runtime', 'options_menu_runtime', 'keyboard_read', 'cd_audio', 'far_depth', 'alpha_pixels', 'transition_gate', 'presentation', 'widescreen', 'orbit_camera', 'coop_camera', 'coop_runtime', 'frame_interpolation', 'frame_runtime', 'first_person', 'first_person_runtime', 'first_person_props', 'first_person_floor', 'camera_runtime', 'mouse_combat', 'camera_collision', 'modern_input', 'modern_runtime', 'analog_movement', 'analog_runtime'))
$ErrorActionPreference = 'Stop'
$ffRoot = Split-Path $PSScriptRoot -Parent
. (Join-Path $ffRoot 'scripts\devshell.ps1')
New-Item -ItemType Directory -Path (Join-Path $ffRoot 'build') -Force | Out-Null
$ffTestTemp = Join-Path $ffRoot ('build/test-temp-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $ffTestTemp | Out-Null
$ffPreviousTemp = $env:TEMP
$ffPreviousTmp = $env:TMP
Push-Location (Join-Path $ffRoot 'build')
try {
    # Native file tests need a writable atomic-rename destination. Keep their
    # temporary state inside the ignored workspace build directory.
    $env:TEMP = $ffTestTemp
    $env:TMP = $ffTestTemp
    foreach ($ffSuite in $Suites) {
        if ($ffSuite -notmatch '^[a-z_]+$') { throw 'Invalid test suite name' }
        $ffSource = Join-Path $PSScriptRoot ($ffSuite + '_test.cpp')
        if (-not (Test-Path -LiteralPath $ffSource)) { throw ('Test source missing: ' + $ffSuite) }
        & cl.exe /nologo /MT /EHsc /W4 $ffSource ('/Fe:' + $ffSuite + '_test.exe') /link user32.lib dxguid.lib dinput8.lib
        if ($LASTEXITCODE -ne 0) { throw ('Test compilation failed: ' + $ffSuite) }
        & (Join-Path '.' ($ffSuite + '_test.exe'))
        if ($LASTEXITCODE -ne 0) { throw ('Test failed: ' + $ffSuite) }
    }
} finally {
    $env:TEMP = $ffPreviousTemp
    $env:TMP = $ffPreviousTmp
    Pop-Location
}
