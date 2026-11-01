param(
    [ValidatePattern('^[0-9]+\.[0-9]+\.[0-9]+(-(alpha|beta|rc)\.[0-9]+)?$')]
    [string]$Version = '0.1.0-rc.1'
)
$ErrorActionPreference = 'Stop'
$ffRoot = Split-Path $PSScriptRoot -Parent
Push-Location $ffRoot
try {
    if (@(& git status --porcelain --untracked-files=normal).Count) { throw 'Commit reviewed changes before building a release.' }
    $ffRevision = (& git rev-parse HEAD).Trim()
    & python scripts/check_repository.py --history
    if ($LASTEXITCODE -ne 0) { throw 'Repository history audit failed.' }
    & python tests/repository_policy_test.py
    if ($LASTEXITCODE -ne 0) { throw 'Repository policy tests failed.' }
    & python tests/release_package_test.py
    if ($LASTEXITCODE -ne 0) { throw 'Release package tests failed.' }
    & (Join-Path $ffRoot 'build.ps1')
    & (Join-Path $ffRoot 'tests/run.ps1')
    & (Join-Path $PSScriptRoot 'build_installer.ps1')
    # Read the managed resource without executing the installer or loading a game.
    $ffSetup = Join-Path $ffRoot 'dist/FightingForceSetup.exe'
    $ffAssembly = [Reflection.Assembly]::Load([IO.File]::ReadAllBytes($ffSetup))
    $ffResource = $ffAssembly.GetManifestResourceStream('FightingForce.Payload.zip')
    if (-not $ffResource) { throw 'Installer payload resource is missing.' }
    $ffPayload = Join-Path $ffRoot 'build/release-payload.zip'
    $ffOutput = [IO.File]::Create($ffPayload)
    try { $ffResource.CopyTo($ffOutput) } finally { $ffOutput.Dispose(); $ffResource.Dispose() }
    & python scripts/package_release.py --version $Version --revision $ffRevision --payload $ffPayload --installer $ffSetup
    if ($LASTEXITCODE -ne 0) { throw 'Release packaging failed.' }
} finally { Pop-Location }
