# Architecture

This document describes the intended system design for `requisite-visualization`. It separates current implementation from planned integration work.

## Current Implementation

The project currently has three implemented pieces:

- A Python catalog generator at `scripts/generate_courses_csv.py`.
- A generated catalog file at `backend/data/courses.csv`.
- A C++ prototype that reads the CSV and builds an in-memory prerequisite graph.

The executable in `backend/src/main.cpp` is a demo/test harness. It is not yet an API server or application runtime.

PostgreSQL schema and Docker Compose configuration exist, but the database currently starts with sample seed data only. The full generated catalog is not imported into PostgreSQL yet.

The `frontend/` directory is a placeholder. No React app has been scaffolded yet.

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

Implemented today:

- Coursedog catalog fetch and CSV generation.
- CSV loading into the C++ graph prototype.
- Flattened graph traversal for simple path checks.

Planned:

- Full PostgreSQL import from `backend/data/courses.csv`.
- Catalog/query abstraction that loads course data once.
- HTTP API for frontend access.
- React/TypeScript course explorer and visualization.

## Backend Graph Layer

The graph layer should support two related representations:

- Grouped prerequisite semantics for correctness.
- Flattened graph edges for traversal and visualization.

Grouped semantics are needed because an OR group means one option is required, not every listed option. Flattened edges are still useful for graph neighborhoods, dependent-course lookups, shortest paths, and cycle detection.

Planned graph queries include:

- Direct prerequisites.
- Recursive prerequisites.
- Direct dependents.
- Recursive dependents.
- Shortest path with path reconstruction.
- Graph neighborhood by course, direction, and depth.
- Cycle detection.

## Database Layer

Current database state:

- Docker Compose can start PostgreSQL.
- The migration defines tables for courses and grouped prerequisites.
- The seed file contains only a small sample dataset.

Planned database work:

- Import all generated CSV courses.
- Preserve prerequisite group order and option order.
- Keep external prerequisite references visible instead of silently dropping them.
- Improve credits modeling for blank, variable, or nonstandard credit values.
- Store enough source metadata to understand when and how the catalog was generated.

Until the import pipeline exists, documentation and application code should not imply that PostgreSQL contains the full catalog.

## Proposed API Boundary

The API has not been implemented. A small proposed endpoint set is:

```text
GET /courses
GET /courses/:id
GET /courses/:id/prerequisites
GET /courses/:id/dependents
GET /graph?course=CMPSC%2016&direction=both&depth=3
GET /paths?from=CMPSC%208&to=CMPSC%20189A
```

Proposed response contracts should distinguish:

- Course metadata.
- Prerequisite groups.
- Graph nodes.
- Graph edges.
- Path results.
- External prerequisite references.

The API strategy is still an open decision. Options include a C++ API around the existing graph layer or a web-native backend that reuses the same catalog/import contract.

## Frontend Direction

The frontend has not been scaffolded. The planned frontend is React + TypeScript.

The first usable screen should be the course explorer itself, not a marketing landing page. Expected initial capabilities:

- Course search.
- Selected-course details.
- Prerequisite and dependent views.
- Graph depth and direction controls.
- Visual distinction between required prerequisites and alternative groups.
- Loading, empty, and error states.

Frontend work should start with mocked API responses until the API contracts stabilize.

## Cross-Lane Handoffs

- Database/import work should define how external prerequisites and credits are represented before the API treats those fields as stable.
- API work should publish small JSON examples before frontend implementation depends on them.
- Graph work should preserve grouped prerequisite data even if traversal uses flattened edges.
- Documentation should describe planned behavior as planned until the relevant lane implements and verifies it.
