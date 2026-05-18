# requisite-visualization

`requisite-visualization` is an early prototype for exploring UCSB College of Engineering course prerequisites. The goal is to help students search for a course, understand direct and recursive prerequisites, see which later courses depend on it, and eventually plan paths through a course or degree sequence.

## Current Behavior

- The C++ backend reads `backend/data/courses.csv` and builds an in-memory prerequisite graph.
- `backend/src/main.cpp` is currently a demo/test harness with hardcoded path checks, not the final app runtime.
- `scripts/generate_courses_csv.py` fetches UCSB Coursedog catalog data and writes the generated CSV.
- PostgreSQL schema and Docker Compose setup exist, but the database currently uses sample seed data and does not load the full catalog.
- The app reads database configuration from environment variables, but the C++ backend does not yet connect to PostgreSQL for catalog queries.
- `frontend/` is currently a placeholder. The planned frontend is React + TypeScript, but it has not been scaffolded.

## Planned Product Direction

The intended product is a course explorer where a student can:

- Search for UCSB College of Engineering courses.
- Inspect direct prerequisite groups and recursive prerequisites.
- Inspect direct and recursive dependents.
- See alternatives in OR prerequisite groups without confusing them for required courses.
- Explore graph neighborhoods by depth and direction.
- Eventually mark completed courses and plan a path through future courses.

## Intended Data Flow

```text
UCSB Coursedog catalog
  -> scripts/generate_courses_csv.py
  -> backend/data/courses.csv
  -> PostgreSQL import
  -> C++ graph/query layer
  -> API
  -> React/TypeScript visualization
```

Only the Coursedog-to-CSV and CSV-to-C++ graph parts are implemented today. PostgreSQL import, API endpoints, and the frontend visualization are planned work.

## Repository Layout

```text
backend/
  data/                  Generated course CSV
  db/                    PostgreSQL schema and sample seed data
  include/               C++ headers
  src/                   C++ prototype implementation
frontend/                Placeholder for planned React/TypeScript app
scripts/                 Data generation and local helper scripts
docs/                    Architecture and data-quality notes
```

## Local Backend Commands

Build the current C++ prototype:

```powershell
mingw32-make
```

Run the current demo executable:

```powershell
.\build\requisite-visualization.exe
```

Clean build artifacts:

```powershell
mingw32-make clean
```

Regenerate the course CSV from UCSB Coursedog:

```powershell
python .\scripts\generate_courses_csv.py --output backend\data\courses.csv
```

`scripts/generate_courses_csv.py` requires `requests`. Python dependencies are not pinned yet.

## Local PostgreSQL With Docker Compose

This project uses Docker Compose for local PostgreSQL. Do not install system-wide PostgreSQL for this setup.

If Docker Desktop is installed but `docker` is not on PATH, run:

```powershell
.\scripts\enable-docker-path.ps1
```

1. Copy the example environment file and replace the placeholder password values:

   ```powershell
   Copy-Item .env.example .env
   ```

   Keep `.env` local. It contains real credentials and is ignored by Git. Commit only `.env.example` with placeholder values.

2. Start PostgreSQL:

   ```powershell
   docker compose up -d postgres
   ```

3. Check the connection and seed data:

   ```powershell
   .\scripts\test-db-connection.ps1
   ```

4. Stop PostgreSQL while keeping the local database volume:

   ```powershell
   docker compose down
   ```

Resetting the database is destructive because it deletes the local `postgres_data` volume. Ask before running this command:

```powershell
docker compose down -v
```

Connect with `psql` inside the container:

```powershell
docker compose exec postgres sh -lc 'psql -U "$POSTGRES_USER" -d "$POSTGRES_DB"'
```

Schema and sample seed files:

- `backend/db/migrations/001_initial_schema.sql`
- `backend/db/seeds/001_sample_data.sql`

## Prerequisite Semantics

Prerequisites are represented as grouped requirements:

```text
AND course, AND course | OR option, OR option; OR option, OR option
```

Every `all` group must be completed. Every `any` group requires one option from the group. Flattened prerequisite-to-course edges are useful for traversal, but they do not fully represent grouped prerequisite semantics by themselves.

See `docs/data-quality.md` for known parser and catalog caveats.

## Near-Term Roadmap

- Split CSV parsing and prerequisite parsing out of the graph implementation.
- Load the full generated CSV catalog into PostgreSQL with grouped prerequisite rows.
- Define small JSON API contracts for courses, prerequisite groups, graph neighborhoods, and paths.
- Scaffold the React/TypeScript frontend and start with mocked API responses.
- Add focused tests for parser behavior, graph traversal, import behavior, and future frontend interactions.
- Document open product decisions before implementation depends on them.

## Open Decisions

- Should the project remain UCSB College of Engineering only, or expand to all UCSB departments?
- Should PostgreSQL become the runtime source of truth, or should the app continue to load CSV at runtime?
- Should the API be implemented in C++ or with a web-native backend?
- How should external prerequisites, concurrent enrollment, minimum grades, standing requirements, and instructor consent be modeled?
- Is the target a local-only tool, a deployed web app, a class project, or a portfolio project?
