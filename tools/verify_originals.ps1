param(
    [Parameter(Mandatory)][string]$MediaRoot,
    [string]$GameDirectory,
    [string]$ExtractedDirectory,
    [string]$ReportDirectory
)
$ErrorActionPreference = 'Stop'
$taskRoot = Split-Path $PSScriptRoot -Parent
if (-not $GameDirectory) { $GameDirectory = Join-Path $taskRoot 'game' }
$taskGame = $GameDirectory
if (-not $ExtractedDirectory) { $ExtractedDirectory = Join-Path $taskRoot 'work\install-extracted-old' }
$taskExtracted = $ExtractedDirectory
$taskExpected = [ordered]@{}
$taskExpected['fforce.exe'] = Join-Path $taskExtracted 'English\fforce.exe'
$taskExpected['ffsetup.exe'] = Join-Path $taskExtracted 'English\ffsetup.exe'
Get-ChildItem -LiteralPath (Join-Path $taskExtracted 'Dlls') -File | ForEach-Object {
    $taskExpected[$_.Name] = $_.FullName
}
foreach ($taskFolder in @('Wads', 'Intro')) {
    $taskSource = Join-Path $MediaRoot $taskFolder
    Get-ChildItem -LiteralPath $taskSource -Recurse -File | ForEach-Object {
        $taskRelative = $_.FullName.Substring($taskSource.Length + 1)
        $taskExpected[(Join-Path $taskFolder $taskRelative)] = $_.FullName
    }
}
foreach ($taskName in @('readme.txt', 'FFORCE.ICO')) {
    $taskExpected[$taskName] = Join-Path $MediaRoot $taskName
}
$taskRecords = foreach ($taskEntry in $taskExpected.GetEnumerator()) {
    $taskSourceHash = (Get-FileHash -LiteralPath $taskEntry.Value -Algorithm SHA256).Hash
    $taskTargetHash = (Get-FileHash -LiteralPath (Join-Path $taskGame $taskEntry.Key) -Algorithm SHA256).Hash
    [PSCustomObject]@{File=$taskEntry.Key;OriginalSHA256=$taskSourceHash;
        InstalledSHA256=$taskTargetHash;Unchanged=($taskSourceHash -eq $taskTargetHash)}
}
if (-not $ReportDirectory) { $ReportDirectory = Join-Path $taskRoot 'verification' }
$taskOutput = $ReportDirectory
New-Item -ItemType Directory -Path $taskOutput -Force | Out-Null
$taskRecords | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath (Join-Path $taskOutput 'original-files.json') -Encoding utf8
$taskFailures = @($taskRecords | Where-Object {-not $_.Unchanged})
[PSCustomObject]@{OriginalFilesChecked=@($taskRecords).Count;ChangedOriginalFiles=$taskFailures.Count;
    ExecutableSHA256=(Get-FileHash -LiteralPath (Join-Path $taskGame 'fforce.exe') -Algorithm SHA256).Hash;
    Report='original-files.json'} | ConvertTo-Json
if ($taskFailures.Count) { throw ('Original file mismatch: ' + ($taskFailures.File -join ', ')) }
