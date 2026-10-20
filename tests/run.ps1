param([string[]]$Suites = @('keyboard_read', 'cd_audio', 'far_depth', 'alpha_pixels'))
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
        & cl.exe /nologo /MT /EHsc /W4 $ffSource ('/Fe:' + $ffSuite + '_test.exe')
        if ($LASTEXITCODE -ne 0) { throw ('Test compilation failed: ' + $ffSuite) }
        & (Join-Path '.' ($ffSuite + '_test.exe'))
        if ($LASTEXITCODE -ne 0) { throw ('Test failed: ' + $ffSuite) }
    }
} finally { Pop-Location }
