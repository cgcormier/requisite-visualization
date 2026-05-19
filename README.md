# requisite-visualization

`requisite-visualization` is a local UCSB course prerequisite explorer. It loads the current generated UCSB catalog, serves a read-only C++ API, and renders a Vite React + TypeScript graph UI for searching courses, inspecting prerequisite groups, viewing dependents, and exploring graph neighborhoods.

## Current Behavior

- `scripts/generate_courses_csv.py` fetches UCSB Coursedog data and writes `backend/data/courses.csv`.
- `backend/data/courses.csv` currently contains 12,271 UCSB course rows with `college`, `subject`, and `department` metadata.
- The C++ API server loads the CSV into memory and serves course, prerequisite, dependent, and graph responses from `backend/src/api/HttpServer.cpp`.
- `frontend/` is a Vite React + TypeScript app using `fetch()` calls against the backend API. Normal runtime does not use `frontend/src/data/mockCatalog.ts`.
- PostgreSQL schema and Docker Compose setup exist, but the running app does not require PostgreSQL yet.
- `backend/src/main.cpp` remains a small demo executable separate from the API server.

## Run Locally

Build the backend demo and API:

```powershell
mingw32-make
mingw32-make api
```

Start the API server:

```powershell
.\build\requisite-api.exe
```

The API binds to `127.0.0.1` and defaults to port `8080`. Override with:

```powershell
$env:API_PORT='18080'
.\build\requisite-api.exe
```

Start the frontend:

```powershell
cd frontend
$env:VITE_API_BASE_URL='http://127.0.0.1:8080'
npm run dev -- --host 127.0.0.1 --port 5173
```

Open `http://127.0.0.1:5173`.

## Useful Checks

```powershell
mingw32-make test-cpp
python -m unittest discover -s tests/python
mingw32-make test-api-smoke
python .\scripts\import_courses_to_postgres.py --dry-run
cd frontend
npm run build
```

The Vite build currently reports a large Cytoscape chunk warning. That warning is expected until bundle splitting is addressed.

## API

Implemented read-only endpoints:

```text
GET /health
GET /courses?q=&subjects=&colleges=&limit=
GET /courses/:id
GET /courses/:id/prerequisites
GET /courses/:id/dependents
GET /graph?course=&direction=&depth=&subjects=&colleges=
```

Responses include `id`, `name`, `credits`, `college`, `department`, `subject`, grouped prerequisites, `groupType`, `groupIndex`, and external prerequisite flags. See `backend/api/API_STRATEGY.md` for examples and current contract details.

## Catalog Generation

Regenerate the current all-UCSB catalog:

```powershell
python .\scripts\generate_courses_csv.py --output backend\data\courses.csv
```

Generate only selected college labels:

```powershell
python .\scripts\generate_courses_csv.py --college "College of Engineering" --output $env:TEMP\courses_coe.csv
```

`scripts/generate_courses_csv.py` requires `requests`. Python dependencies are not pinned yet.

## PostgreSQL

PostgreSQL is optional for the current app runtime. Docker Compose and schema files are available for import work:

- `backend/db/migrations/001_initial_schema.sql`
- `backend/db/seeds/001_sample_data.sql`

Start local PostgreSQL:

```powershell
docker compose up -d postgres
```

Check connectivity:

```powershell
.\scripts\test-db-connection.ps1
```

Do not run `docker compose down -v` unless you intentionally want to delete the local `postgres_data` volume.

## Prerequisite Semantics

Prerequisites are represented as grouped requirements:

```text
AND course, AND course | OR option, OR option; OR option, OR option
```

Every `all` group must be completed. Every `any` group requires one option from that group. Flattened prerequisite-to-course edges are useful for traversal and graph display, but the grouped data remains the source for prerequisite semantics.

## Remaining Work

- Decide whether PostgreSQL should become the runtime source of truth.
- Add `/paths` and path reconstruction to the API.
- Improve parser handling for concurrent enrollment, minimum grades, standing, instructor consent, and non-course requirements.
- Add frontend tests beyond the current build and browser smoke workflow.
- Split the Cytoscape bundle if production bundle size becomes important.
