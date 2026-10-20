$ErrorActionPreference = 'Stop'
if ($env:VSCMD_ARG_TGT_ARCH -eq 'x86' -and (Get-Command cl.exe -ErrorAction SilentlyContinue)) { return }

$ffInstallerRoot = ${env:ProgramFiles(x86)}
if (-not $ffInstallerRoot) { $ffInstallerRoot = $env:ProgramFiles }
$ffVsWhere = Join-Path $ffInstallerRoot 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $ffVsWhere)) {
    throw 'Visual Studio Installer was not found. Install the C++ build tools and Windows SDK.'
}
$ffVsRoot = & $ffVsWhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $ffVsRoot) { throw 'Visual Studio C++ build tools were not found.' }
Import-Module (Join-Path $ffVsRoot 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll')
Enter-VsDevShell -VsInstallPath $ffVsRoot -SkipAutomaticLocation -DevCmdArguments '-arch=x86 -host_arch=x64' | Out-Null
