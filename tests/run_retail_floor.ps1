param([string]$Executable = (Join-Path (Split-Path $PSScriptRoot -Parent) 'game\fforce.exe'))
$ErrorActionPreference='Stop'
$ffImage=(Resolve-Path -LiteralPath $Executable).Path
$ffSupportedImages=@(
    '92AB6179BF6497D471C6AB30D6DF6F4F333D5BC3F641A154D6966B47FDBA90A7',
    'BF1BD02DC9B6508EDD76EFA36E39A6CC1B82055F7D31CCDA329C1AE55796FDA3'
)
if((Get-FileHash -LiteralPath $ffImage -Algorithm SHA256).Hash -notin $ffSupportedImages) {
    throw 'Unsupported game image for isolated floor validation'
}
$ffRoot=Split-Path $PSScriptRoot -Parent
. (Join-Path $ffRoot 'scripts\devshell.ps1')
New-Item -ItemType Directory -Path (Join-Path $ffRoot 'build') -Force | Out-Null
Push-Location (Join-Path $ffRoot 'build')
try {
    & cl.exe /nologo /MT /EHsc /W4 /DFF_RETAIL_FLOOR (Join-Path $PSScriptRoot 'first_person_floor_test.cpp') /Fe:retail_floor_test.exe /link /BASE:0x10000000 /DYNAMICBASE
    if($LASTEXITCODE -ne 0) {throw 'Retail floor test compilation failed'}
    for($ffAttempt=0;$ffAttempt -lt 15;$ffAttempt++) {
        & '.\retail_floor_test.exe' $ffImage
        if($LASTEXITCODE -ne 3) {break}
    }
    if($LASTEXITCODE -ne 0) {throw 'Retail floor test failed'}
} finally {Pop-Location}
