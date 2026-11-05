$ErrorActionPreference = 'Stop'
$ffRoot = Split-Path $PSScriptRoot -Parent
$ffTest = Join-Path $ffRoot ('build/room-review-test-' + [Guid]::NewGuid().ToString('N'))
& python (Join-Path $ffRoot 'tools/prepare_room_review.py') --presets-only --output $ffTest
if ($LASTEXITCODE -ne 0) { throw 'Fixture generation failed' }
Copy-Item -LiteralPath (Join-Path $ffRoot 'tools/room_review_launcher.ps1') -Destination (Join-Path $ffTest 'Review-Rooms.ps1')
$ffGame = Join-Path $ffTest 'Game'
New-Item -ItemType Directory -Path $ffGame | Out-Null
# PrepareOnly must never execute this placeholder.
[IO.File]::WriteAllBytes((Join-Path $ffGame 'fforce.exe'), [byte[]](1,2,3))
$ffSave = Join-Path $ffGame 'fightingforce-save-3.sav'
[IO.File]::WriteAllBytes($ffSave, [byte[]](42,43))
[IO.File]::WriteAllBytes(($ffSave + '.bak'), [byte[]](44,45))
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $ffTest 'Review-Rooms.ps1') -Pack 11 -PrepareOnly
if ($LASTEXITCODE -ne 0) { throw 'Review set preparation failed' }
if ((Test-Path -LiteralPath $ffSave) -or (Test-Path -LiteralPath ($ffSave + '.bak'))) { throw 'Stale slot survived a one-slot set' }
$ffExpected = (Get-FileHash -LiteralPath (Join-Path $ffTest 'presets/stage-25.sav')).Hash
if ((Get-FileHash -LiteralPath (Join-Path $ffGame 'fightingforce-save-1.sav')).Hash -ne $ffExpected) { throw 'Wrong stage installed' }
$ffSaved = @(Get-ChildItem -LiteralPath (Join-Path $ffTest 'history') -Recurse -File)
if ($ffSaved.Count -ne 2) { throw 'Missing progress backup' }
foreach ($ffFile in $ffSaved) {
    $ffBytes = [IO.File]::ReadAllBytes($ffFile.FullName)
    if ($ffBytes.Length -ne 2 -or $ffBytes[0] -notin @(42,44)) { throw 'Progress backup changed' }
}
Write-Output 'Room review set switching: PASS'
