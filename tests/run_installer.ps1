param([Parameter(Mandatory)][string]$MediaRoot,[string]$GameFolder)
$ErrorActionPreference = 'Stop'
$ffRoot = Split-Path $PSScriptRoot -Parent
& (Join-Path $PSScriptRoot 'run_installer_core.ps1') -MediaRoot $MediaRoot
if ($GameFolder) { & (Join-Path $PSScriptRoot 'run_installer_core.ps1') -MediaRoot $GameFolder }
$ffArguments = @((Join-Path $PSScriptRoot 'installer_integration_test.py'),'--media',$MediaRoot)
if ($GameFolder) { $ffArguments += @('--game-folder',$GameFolder) }
& python @ffArguments
if ($LASTEXITCODE -ne 0) { throw 'Installer integration checks failed.' }
