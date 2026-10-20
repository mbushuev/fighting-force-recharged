$ErrorActionPreference = 'Stop'
$taskRoot = $PSScriptRoot
. (Join-Path $taskRoot 'scripts\devshell.ps1')
$taskBuild = Join-Path $taskRoot 'build'
New-Item -ItemType Directory -Path $taskBuild -Force | Out-Null
Push-Location $taskBuild
try {
    & cl.exe /nologo /LD /MT /O2 /W4 /EHsc /DUNICODE /D_UNICODE `
        (Join-Path $taskRoot 'src\dinput_proxy.cpp') `
        /link ('/DEF:' + (Join-Path $taskRoot 'src\dinput_proxy.def')) `
        /OUT:DINPUT.dll /MACHINE:X86 kernel32.lib user32.lib gdi32.lib advapi32.lib dxguid.lib ole32.lib
    if ($LASTEXITCODE -ne 0) { throw 'Compiler failed' }
    Write-Output (Join-Path $taskBuild 'DINPUT.dll')
} finally {
    Pop-Location
}
