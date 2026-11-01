param([string]$OutputDirectory)
$ErrorActionPreference = 'Stop'
$ffRoot = Split-Path $PSScriptRoot -Parent
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $ffRoot 'build/dependencies' }
if (Test-Path -LiteralPath $OutputDirectory) { throw 'Dependency destination already exists; use a fresh directory.' }
$ffDownloads = Get-Content -LiteralPath (Join-Path $ffRoot 'config/build-downloads.json') -Raw | ConvertFrom-Json
$ffDependencies = Get-Content -LiteralPath (Join-Path $ffRoot 'config/installer-dependencies.json') -Raw | ConvertFrom-Json
if ($ffDownloads.schema -ne 1) { throw 'Unsupported dependency-download inventory.' }
New-Item -ItemType Directory -Path $OutputDirectory | Out-Null
$ffRuntime = Join-Path $OutputDirectory 'runtime'
$ffExtractor = Join-Path $OutputDirectory 'unshield'
New-Item -ItemType Directory -Path $ffRuntime,$ffExtractor | Out-Null
foreach ($ffName in @('DDraw.dll','D3DImm.dll','D3D9.dll')) {
    $ffPath = Join-Path $ffRuntime $ffName
    Invoke-WebRequest -Uri ($ffDownloads.runtime_base + $ffName) -OutFile $ffPath
}
$ffArchive = Join-Path $OutputDirectory 'unshield.7z'
Invoke-WebRequest -Uri $ffDownloads.extractor.url -OutFile $ffArchive
if ((Get-Item -LiteralPath $ffArchive).Length -ne $ffDownloads.extractor.size -or
    (Get-FileHash -LiteralPath $ffArchive -Algorithm SHA256).Hash.ToLowerInvariant() -ne $ffDownloads.extractor.sha256) {
    throw 'Extractor archive differs from the reviewed download.'
}
# Hosted Windows 2022 has 7-Zip, but its older tar lacks LZMA support. Prefer
# the installed extractor; recent Windows tar is a dependency-free fallback.
# Only the two reviewed root members are selected. Downloads are not executed.
$ffSevenZipCommand = Get-Command 7z.exe -ErrorAction SilentlyContinue
$ffSevenZip = if ($ffSevenZipCommand) { $ffSevenZipCommand.Source } else { $null }
if (-not $ffSevenZip) {
    $ffInstalled7z = Join-Path $env:ProgramFiles '7-Zip/7z.exe'
    if (Test-Path -LiteralPath $ffInstalled7z -PathType Leaf) { $ffSevenZip = $ffInstalled7z }
}
if ($ffSevenZip) {
    & $ffSevenZip e -y $ffArchive "-o$ffExtractor" unshield.exe zlib1.dll
} else {
    & tar.exe -xf $ffArchive -C $ffExtractor unshield.exe zlib1.dll
}
if ($LASTEXITCODE -ne 0) { throw 'Extractor unpack failed; install 7-Zip or use Windows tar with LZMA support.' }
foreach ($ffDependency in $ffDependencies) {
    $ffFolder = if ($ffDependency.name -in @('unshield.exe','zlib1.dll')) { $ffExtractor } else { $ffRuntime }
    $ffPath = Join-Path $ffFolder $ffDependency.name
    if ((Get-Item -LiteralPath $ffPath).Length -ne $ffDependency.size -or
        (Get-FileHash -LiteralPath $ffPath -Algorithm SHA256).Hash.ToLowerInvariant() -ne $ffDependency.sha256) {
        throw ('Downloaded dependency differs: ' + $ffDependency.name)
    }
}
Write-Output 'Build dependencies downloaded and verified; no game data was requested.'
