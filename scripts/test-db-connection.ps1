$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$envFile = Join-Path $repoRoot ".env"
$containerName = "requisite-visualization-postgres"

if (-not (Get-Command docker -ErrorAction SilentlyContinue)) {
    throw "Docker CLI was not found. Install Docker Desktop or make docker available on PATH."
}

if (-not (Test-Path $envFile)) {
    Write-Warning "No .env file found. Copy .env.example to .env and replace placeholder values before starting Postgres."
}

Push-Location $repoRoot
try {
    $runningContainer = docker ps --filter "name=^/$containerName$" --format "{{.Names}}"
    if ($runningContainer -ne $containerName) {
        throw "Container '$containerName' is not running. Start it with: docker compose up -d postgres"
    }

    $postgresUser = docker exec $containerName printenv POSTGRES_USER
    $postgresDb = docker exec $containerName printenv POSTGRES_DB

    docker exec $containerName pg_isready -U $postgresUser -d $postgresDb
    if ($LASTEXITCODE -ne 0) {
        throw "PostgreSQL is not ready yet."
    }

    docker exec $containerName psql -U $postgresUser -d $postgresDb -v ON_ERROR_STOP=1 -P pager=off `
        -c "select current_database() as database_name, count(*) as course_count from courses;" `
        -c "select count(*) as prerequisite_group_count from course_prerequisite_groups;" `
        -c "select count(*) as prerequisite_option_count from course_prerequisite_options;"
    if ($LASTEXITCODE -ne 0) {
        throw "Database connection test failed. Start the service with: docker compose up -d postgres"
    }
} finally {
    Pop-Location
}
