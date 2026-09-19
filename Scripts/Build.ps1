[CmdletBinding()]
param(
    [ValidateSet('Editor', 'Game')]
    [string]$Target = 'Editor',

    [ValidateSet('Development', 'DebugGame', 'Shipping')]
    [string]$Configuration = 'Development',

    [string]$EngineRoot = $env:UE_ENGINE_ROOT
)

$ErrorActionPreference = 'Stop'

if ($Target -eq 'Editor' -and $Configuration -eq 'Shipping') {
    throw 'Shipping is not an Editor configuration. Use -Target Game.'
}

$projectRoot = Split-Path -Parent $PSScriptRoot
$projectFile = Join-Path $projectRoot 'ProjectKata.uproject'
$projectDescriptor = Get-Content -LiteralPath $projectFile -Raw | ConvertFrom-Json
$engineVersion = $projectDescriptor.EngineAssociation

if ([string]::IsNullOrWhiteSpace($EngineRoot)) {
    $engineRegistration = Get-ItemProperty -LiteralPath "HKLM:\SOFTWARE\EpicGames\Unreal Engine\$engineVersion" -ErrorAction SilentlyContinue
    if ($null -ne $engineRegistration) {
        $EngineRoot = $engineRegistration.InstalledDirectory
    }
}

if ([string]::IsNullOrWhiteSpace($EngineRoot)) {
    $EngineRoot = Join-Path $env:ProgramFiles "Epic Games\UE_$engineVersion"
}

$buildScript = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
if (-not (Test-Path -LiteralPath $buildScript -PathType Leaf)) {
    throw "Unreal Build.bat was not found. Set -EngineRoot or UE_ENGINE_ROOT to the UE $engineVersion installation directory."
}

$buildTarget = if ($Target -eq 'Editor') { 'ProjectKataEditor' } else { 'ProjectKata' }

& $buildScript $buildTarget Win64 $Configuration "-Project=$projectFile" -WaitMutex -NoHotReloadFromIDE -NoLiveCoding
if ($LASTEXITCODE -ne 0) {
    throw "Unreal build failed with exit code $LASTEXITCODE."
}
