# Architecture

This document describes the current local architecture for `requisite-visualization` and separates implemented behavior from remaining roadmap work.

## Implemented Data Flow

```text
UCSB Coursedog catalog
  -> scripts/generate_courses_csv.py
  -> backend/data/courses.csv
  -> in-memory C++ catalog/API
  -> React/TypeScript visualization
```

PostgreSQL schema and import tooling exist, but PostgreSQL is not currently required by the running app.

## Catalog Layer

`scripts/generate_courses_csv.py` fetches current UCSB Coursedog records and writes:

```text
id,name,credits,college,prereqs,subject,department
```

The first five columns remain compatible with older graph code. `subject` and `department` support API responses and frontend filters. The generator can filter by exact college label with `--college`, and the default output is all current UCSB courses for the chosen effective date.

## Backend API

The API server is implemented in C++ under:

- `backend/include/api/`
- `backend/src/api/`

`backend/src/api/HttpServer.cpp` is a small standalone local HTTP server. It binds to `127.0.0.1`, defaults to `API_PORT=8080`, loads `backend/data/courses.csv`, and does not read `.env`.

Implemented endpoints:

```text
GET /health
GET /courses
GET /courses/:id
GET /courses/:id/prerequisites
GET /courses/:id/dependents
GET /graph?course=CMPSC%2016&direction=both&depth=3
```

The API preserves grouped prerequisite semantics with `groupType` and `groupIndex`, while also returning flattened IDs and graph edges for visualization. External prerequisite references remain visible with `external` flags.

## Frontend

`frontend/` is a Vite React + TypeScript app using Cytoscape for graph visualization. It uses `fetch()` through `frontend/src/api/client.ts`; normal runtime does not import `frontend/src/data/mockCatalog.ts`.

Implemented UI behavior:

- Course search and selected-course detail from backend API data.
- Prerequisite and dependent sections from backend relationship endpoints.
- Graph neighborhoods from `/graph`.
- Multi-select college filters and subject filtering.
- Dark high-contrast workspace.
- Circular course nodes.
- Clickable graph nodes that refetch and display course details.
- Fit, zoom in, zoom out, reset, and fullscreen controls.
- Solid bright colors for `any` prerequisite groups, keyed by `groupIndex`.

## Database Layer

Current database state:

- Docker Compose can start PostgreSQL.
- The migration defines tables for courses and grouped prerequisites.
- The seed file contains a small sample dataset.
- `scripts/import_courses_to_postgres.py` supports dry-run validation of the expanded CSV and accepts optional `subject` and `department` columns.

Remaining database work:

- Decide whether PostgreSQL should become the runtime source of truth.
- Add schema columns or related tables for subject and department if PostgreSQL stores the full catalog.
- Preserve external prerequisite references and nonstandard credit values in a durable model.

## Open Architecture Decisions

- CSV runtime source vs. PostgreSQL runtime source.
- `/paths` endpoint and path reconstruction ownership.
- Long-term HTTP server strategy if the local socket server grows too much.
- Modeling concurrent enrollment, minimum grades, standing requirements, instructor consent, and non-course prerequisites.
- Frontend test strategy for Cytoscape interactions.
