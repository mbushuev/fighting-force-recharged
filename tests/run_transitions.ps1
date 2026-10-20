$ErrorActionPreference = 'Stop'
$ffRoot = Split-Path $PSScriptRoot -Parent
. (Join-Path $ffRoot 'scripts\devshell.ps1')
New-Item -ItemType Directory -Path (Join-Path $ffRoot 'build') -Force | Out-Null
Push-Location (Join-Path $ffRoot 'build')
try {
    & cl.exe /nologo /MT /O2 /EHsc /W4 (Join-Path $PSScriptRoot 'transition_window_test.cpp') /Fe:transition_window_test.exe /link user32.lib gdi32.lib
    if($LASTEXITCODE -ne 0){throw 'Transition window test compilation failed'}
    & '.\transition_window_test.exe'
    if($LASTEXITCODE -ne 0){throw 'Transition window test failed'}
} finally { Pop-Location }
