param([string]$RuntimeDirectory,[string]$UnshieldDirectory,[string]$OutputDirectory)
$ErrorActionPreference = 'Stop'
$ffRoot = Split-Path $PSScriptRoot -Parent
if (-not $RuntimeDirectory) { $RuntimeDirectory = Join-Path $ffRoot 'game' }
if (-not $UnshieldDirectory) { $UnshieldDirectory = Join-Path $ffRoot 'tools\unshield' }
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $ffRoot 'dist' }
$ffBuild = Join-Path $ffRoot 'build\installer'
$ffPayload = Join-Path $ffBuild ('payload-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $ffPayload,$OutputDirectory -Force | Out-Null
foreach ($ffName in @('game','tools','support','support\licenses')) {
    New-Item -ItemType Directory -Path (Join-Path $ffPayload $ffName) -Force | Out-Null
}
$ffDll = Join-Path $ffRoot 'build\DINPUT.dll'
if (-not (Test-Path -LiteralPath $ffDll)) { throw 'Build the compatibility DLL first with build.ps1.' }
Copy-Item -LiteralPath $ffDll -Destination (Join-Path $ffPayload 'game\DINPUT.dll')
foreach ($ffName in @('fightingforce-compat.ini','dgVoodoo.conf')) {
    Copy-Item -LiteralPath (Join-Path $ffRoot ('config\' + $ffName)) -Destination (Join-Path $ffPayload ('game\' + $ffName))
}
$ffDependencies = Get-Content -LiteralPath (Join-Path $ffRoot 'config\installer-dependencies.json') -Raw | ConvertFrom-Json
foreach ($ffDependency in $ffDependencies) {
    $ffExtractor = $ffDependency.name -in @('unshield.exe','zlib1.dll')
    $ffSource = Join-Path $(if ($ffExtractor) { $UnshieldDirectory } else { $RuntimeDirectory }) $ffDependency.name
    if (-not (Test-Path -LiteralPath $ffSource)) { throw ('Missing reviewed dependency: ' + $ffDependency.name) }
    if ((Get-Item -LiteralPath $ffSource).Length -ne $ffDependency.size -or
        (Get-FileHash -LiteralPath $ffSource -Algorithm SHA256).Hash.ToLowerInvariant() -ne $ffDependency.sha256) {
        throw ('Dependency differs from the reviewed version: ' + $ffDependency.name)
    }
    $ffDestination = Join-Path $ffPayload $(if ($ffExtractor) { 'tools' } else { 'game' })
    Copy-Item -LiteralPath $ffSource -Destination $ffDestination
}
foreach ($ffName in @('INSTALLER.md','CONTROLS.md','FRAME-RATE.md','COOPERATIVE.md','INSTALLER-DEPENDENCIES.md','VALIDATION.md')) {
    Copy-Item -LiteralPath (Join-Path $ffRoot ('docs\' + $ffName)) -Destination (Join-Path $ffPayload 'support')
}
Get-ChildItem -LiteralPath (Join-Path $ffRoot 'docs\licenses') -File | ForEach-Object {
    Copy-Item -LiteralPath $_.FullName -Destination (Join-Path $ffPayload 'support\licenses')
}
Copy-Item -LiteralPath (Join-Path $ffRoot 'config\retail-english.json') -Destination (Join-Path $ffPayload 'support')
Copy-Item -LiteralPath (Join-Path $ffRoot 'config\rip-english.json') -Destination (Join-Path $ffPayload 'support')
$ffZip = Join-Path $ffBuild ('payload-' + [Guid]::NewGuid().ToString('N') + '.zip')
Add-Type -AssemblyName System.IO.Compression.FileSystem
[IO.Compression.ZipFile]::CreateFromDirectory($ffPayload,$ffZip,[IO.Compression.CompressionLevel]::Optimal,$false)
$ffCompiler = Join-Path $env:WINDIR 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'
if (-not (Test-Path -LiteralPath $ffCompiler)) { throw 'The Windows .NET Framework 4 compiler is required to build setup.' }
$ffOutput = Join-Path $OutputDirectory 'FightingForceSetup.exe'
& $ffCompiler /nologo /target:winexe /platform:x64 /optimize+ /warn:4 `
    /reference:System.Windows.Forms.dll /reference:System.Drawing.dll /reference:System.Web.Extensions.dll `
    /reference:System.IO.Compression.dll /reference:System.IO.Compression.FileSystem.dll `
    ('/resource:' + $ffZip + ',FightingForce.Payload.zip') `
    ('/resource:' + (Join-Path $ffRoot 'config\retail-english.json') + ',FightingForce.Retail.json') `
    ('/resource:' + (Join-Path $ffRoot 'config\rip-english.json') + ',FightingForce.Rip.json') `
    ('/win32manifest:' + (Join-Path $ffRoot 'tools\installer\setup.manifest')) `
    ('/out:' + $ffOutput) (Join-Path $ffRoot 'tools\installer\InstallerCore.cs') (Join-Path $ffRoot 'tools\installer\FolderFix.cs') (Join-Path $ffRoot 'tools\installer\Setup.cs')
if ($LASTEXITCODE -ne 0) { throw 'Installer compilation failed.' }
Get-FileHash -LiteralPath $ffOutput -Algorithm SHA256 | ForEach-Object {
    ($_.Hash.ToLowerInvariant() + '  FightingForceSetup.exe') | Set-Content -LiteralPath (Join-Path $OutputDirectory 'FightingForceSetup.sha256') -Encoding ascii
}
Write-Output $ffOutput
