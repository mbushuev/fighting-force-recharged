param([Parameter(Mandatory)][string]$MediaRoot)
$ErrorActionPreference = 'Stop'
$ffRoot = Split-Path $PSScriptRoot -Parent
& (Join-Path $PSScriptRoot 'run_installer_core.ps1') -MediaRoot $MediaRoot
& python (Join-Path $PSScriptRoot 'installer_integration_test.py') --media $MediaRoot
if ($LASTEXITCODE -ne 0) { throw 'Installer integration checks failed.' }
