param([string]$RuntimeDirectory)
$ErrorActionPreference = 'Stop'
$ffRoot = Split-Path $PSScriptRoot -Parent
. (Join-Path $ffRoot 'scripts\devshell.ps1')
New-Item -ItemType Directory -Path (Join-Path $ffRoot 'build') -Force | Out-Null
Push-Location (Join-Path $ffRoot 'build')
try {
    & cl.exe /nologo /MT /O2 /EHsc /W4 (Join-Path $PSScriptRoot 'render_gpu_test.cpp') /Fe:render_gpu_test.exe /link user32.lib dxguid.lib ole32.lib
    if ($LASTEXITCODE -ne 0) { throw 'GPU test compilation failed' }
    if (-not $RuntimeDirectory) { $RuntimeDirectory = Join-Path $ffRoot 'game' }
    $ffRuntime = (Resolve-Path -LiteralPath (Join-Path $RuntimeDirectory 'DDraw.dll')).Path
    & '.\render_gpu_test.exe' $ffRuntime
    if ($LASTEXITCODE -ne 0) { throw 'GPU rendering test failed' }
} finally { Pop-Location }
