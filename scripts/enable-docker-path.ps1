$ErrorActionPreference = "Stop"

$knownDockerDirs = @(@(
    "C:\Program Files\Docker\Docker\resources\bin",
    "$env:LOCALAPPDATA\Docker\resources\bin"
) | Where-Object { $_ -and (Test-Path (Join-Path $_ "docker.exe")) })

if (-not $knownDockerDirs) {
    throw "docker.exe was not found. Install Docker Desktop first, then rerun this script."
}

$dockerDir = $knownDockerDirs[0]
$pathParts = $env:Path -split ";" | Where-Object { $_ -and $_ -ne "C" }

if ($pathParts -notcontains $dockerDir) {
    $pathParts = @($dockerDir) + @($pathParts)
}

$env:Path = ($pathParts | Select-Object -Unique) -join ";"

$userPath = [Environment]::GetEnvironmentVariable("Path", "User")
$userPathParts = $userPath -split ";" | Where-Object { $_ -and $_ -ne "C" }

if ($userPathParts -notcontains $dockerDir) {
    $userPathParts = @($dockerDir) + @($userPathParts)
}

$newUserPath = ($userPathParts | Select-Object -Unique) -join ";"
[Environment]::SetEnvironmentVariable("Path", $newUserPath, "User")

& (Join-Path $dockerDir "docker.exe") --version
& (Join-Path $dockerDir "docker.exe") compose version
