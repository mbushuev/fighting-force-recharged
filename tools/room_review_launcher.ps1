param([int]$Pack = -1, [switch]$PrepareOnly)
$ErrorActionPreference = 'Stop'
$ffRoot = $PSScriptRoot
$ffManifest = Get-Content -LiteralPath (Join-Path $ffRoot 'room-review.json') -Raw | ConvertFrom-Json
if ($ffManifest.format -ne 'ff-room-review-1') { throw 'Not a room review directory' }
$ffGame = Join-Path $ffRoot 'Game'
$ffExe = Join-Path $ffGame 'fforce.exe'
if (-not (Test-Path -LiteralPath $ffExe)) { throw 'Review game is missing' }

function Set-ReviewPack([int]$Index) {
    if ($Index -lt 0 -or $Index -ge $ffManifest.packs.Count) { throw 'Invalid review set' }
    if (Get-Process -Name fforce -ErrorAction SilentlyContinue) { throw 'Close the game before switching review saves.' }
    $ffStages = @($ffManifest.packs[$Index].stages)
    if ($ffStages.Count -lt 1 -or $ffStages.Count -gt 3) { throw 'Invalid slot count' }
    foreach ($ffStage in $ffStages) {
        if ($ffStage -lt 0 -or $ffStage -ge 25) { throw 'Invalid stage' }
        $ffPreset = Join-Path $ffRoot ('presets/stage-{0:00}.sav' -f ($ffStage + 1))
        if (-not (Test-Path -LiteralPath $ffPreset)) { throw 'Review preset is missing' }
    }
    # Preserve both primary and recovery records before replacing test progress.
    $ffHistory = Join-Path $ffRoot ('history/' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
    New-Item -ItemType Directory -Path $ffHistory | Out-Null
    $ffSaveNames = @('fightingforce-save.sav', 'fightingforce-save.sav.bak')
    for ($i = 1; $i -le 3; $i++) { $ffSaveNames += "fightingforce-save-$i.sav", "fightingforce-save-$i.sav.bak" }
    foreach ($ffName in $ffSaveNames) {
        $ffPath = Join-Path $ffGame $ffName
        if (Test-Path -LiteralPath $ffPath) { Copy-Item -LiteralPath $ffPath -Destination (Join-Path $ffHistory $ffName) }
    }
    foreach ($ffName in $ffSaveNames) {
        $ffPath = Join-Path $ffGame $ffName
        if (Test-Path -LiteralPath $ffPath) { Remove-Item -LiteralPath $ffPath }
    }
    for ($i = 0; $i -lt $ffStages.Count; $i++) {
        Copy-Item -LiteralPath (Join-Path $ffRoot ('presets/stage-{0:00}.sav' -f ($ffStages[$i] + 1))) -Destination (Join-Path $ffGame ('fightingforce-save-{0}.sav' -f ($i + 1)))
    }
    if (-not $PrepareOnly) { Start-Process -FilePath $ffExe -WorkingDirectory $ffGame }
}

if ($Pack -ge 0) { Set-ReviewPack $Pack; exit }
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
[System.Windows.Forms.Application]::EnableVisualStyles()
$ffForm = New-Object System.Windows.Forms.Form
$ffForm.Text = 'Fighting Force - Room Review'
$ffForm.ClientSize = New-Object System.Drawing.Size(570, 315)
$ffForm.AutoScaleMode = 'Dpi'
$ffForm.Font = New-Object System.Drawing.Font('Segoe UI', 10)
$ffForm.FormBorderStyle = 'FixedDialog'
$ffForm.MaximizeBox = $false
$ffForm.StartPosition = 'CenterScreen'
$ffSelect = New-Object System.Windows.Forms.ComboBox
$ffSelect.SetBounds(20, 20, 530, 32)
$ffSelect.DropDownStyle = 'DropDownList'
foreach ($ffEntry in $ffManifest.packs) { [void]$ffSelect.Items.Add($ffEntry.name) }
$ffInfo = New-Object System.Windows.Forms.Label
$ffInfo.SetBounds(20, 65, 530, 100)
$ffNote = New-Object System.Windows.Forms.Label
$ffNote.SetBounds(20, 170, 530, 80)
$ffNote.Text = 'Start the game, then choose Load Game. Presets begin at the stage entrance; reach later sections normally. Switching sets preserves test progress in the history folder. Close the game first.'
$ffButton = New-Object System.Windows.Forms.Button
$ffButton.SetBounds(330, 263, 220, 34)
$ffButton.Text = 'Load set and start game'
$ffSelect.add_SelectedIndexChanged({
    $ffRows = @('Load Game slots:')
    $ffSelected = @($ffManifest.packs[$ffSelect.SelectedIndex].stages)
    for ($i = 0; $i -lt $ffSelected.Count; $i++) {
        $s = $ffSelected[$i]
        $ffRows += ('{0}: Stage {1} / STAGE{2}' -f ($i + 1), ($s + 1), $ffManifest.stages[$s])
    }
    $ffInfo.Text = $ffRows -join "`r`n"
})
$ffButton.add_Click({
    try { Set-ReviewPack $ffSelect.SelectedIndex; $ffForm.Close() }
    catch { [void][System.Windows.Forms.MessageBox]::Show($_.Exception.Message, 'Room review') }
})
$ffForm.Controls.AddRange(@($ffSelect, $ffInfo, $ffNote, $ffButton))
$ffSelect.SelectedIndex = 0
[void]$ffForm.ShowDialog()
