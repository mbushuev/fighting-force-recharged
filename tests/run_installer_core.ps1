param([string]$MediaRoot)
$ErrorActionPreference = 'Stop'
$ffRoot = Split-Path $PSScriptRoot -Parent
$ffCompiler = Join-Path $env:WINDIR 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'
$ffTestExe = Join-Path $ffRoot 'build\installer_core_test.exe'
New-Item -ItemType Directory -Path (Split-Path $ffTestExe -Parent) -Force | Out-Null
$ffArguments = @('/nologo','/platform:x64','/target:exe','/warn:4',
    '/reference:System.Web.Extensions.dll','/reference:System.IO.Compression.dll',
    '/reference:System.IO.Compression.FileSystem.dll',
    ('/resource:' + (Join-Path $ffRoot 'config\retail-english.json') + ',FightingForce.Retail.json'),
    ('/resource:' + (Join-Path $ffRoot 'config\rip-english.json') + ',FightingForce.Rip.json'),
    ('/out:' + $ffTestExe), (Join-Path $ffRoot 'tools\installer\InstallerCore.cs'),
    (Join-Path $ffRoot 'tools\installer\FolderFix.cs'),
    (Join-Path $PSScriptRoot 'installer_core_test.cs'))
if ($MediaRoot) {
    $ffPayload = Get-ChildItem -LiteralPath (Join-Path $ffRoot 'build\installer') -Filter 'payload-*.zip' |
        Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if (-not $ffPayload) { throw 'Build the installer before running copying cancellation tests.' }
    $ffArguments += '/resource:' + $ffPayload.FullName + ',FightingForce.Payload.zip'
}
& $ffCompiler @ffArguments
if ($LASTEXITCODE -ne 0) { throw 'Installer core test compilation failed.' }
if ($MediaRoot) {
    $ffCancelParent = Join-Path $ffRoot ('work\installer-tests\cancel-' + [Guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $ffCancelParent -Force | Out-Null
    & $ffTestExe $MediaRoot $ffCancelParent
} else {
    & $ffTestExe
}
if ($LASTEXITCODE -ne 0) { throw 'Installer core checks failed.' }
