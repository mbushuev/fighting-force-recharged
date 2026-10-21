$ErrorActionPreference = 'Stop'
$ffRoot = Split-Path $PSScriptRoot -Parent
$ffCompiler = Join-Path $env:WINDIR 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'
$ffTestExe = Join-Path $ffRoot 'build\installer_layout_test.exe'
New-Item -ItemType Directory -Path (Split-Path $ffTestExe -Parent) -Force | Out-Null
& $ffCompiler /nologo /platform:x64 /target:exe /main:InstallerLayoutTests /warn:4 `
    /reference:System.Windows.Forms.dll /reference:System.Drawing.dll /reference:System.Web.Extensions.dll `
    /reference:System.IO.Compression.dll /reference:System.IO.Compression.FileSystem.dll `
    ('/win32manifest:' + (Join-Path $ffRoot 'tools\installer\setup.manifest')) `
    ('/out:' + $ffTestExe) (Join-Path $ffRoot 'tools\installer\InstallerCore.cs') `
    (Join-Path $ffRoot 'tools\installer\FolderFix.cs') `
    (Join-Path $ffRoot 'tools\installer\Setup.cs') (Join-Path $PSScriptRoot 'installer_layout_test.cs')
if ($LASTEXITCODE -ne 0) { throw 'Installer layout test compilation failed.' }
& $ffTestExe
if ($LASTEXITCODE -ne 0) { throw 'Installer layout checks failed.' }
