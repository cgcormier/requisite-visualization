# AGENTS.md

Repository instructions for coding agents working on `requisite-visualization`.

## Project Overview

`requisite-visualization` is an early prototype for exploring UCSB College of Engineering course prerequisites. The intended product is a web app where students can search for a course, see direct and recursive prerequisites, inspect dependent courses, and eventually plan a path through courses or a degree sequence.

Current state:

- The C++ backend reads `backend/data/courses.csv` and builds a prerequisite graph.
- PostgreSQL schema and Docker Compose setup exist, but the app does not yet load the full catalog into PostgreSQL.
- The executable in `backend/src/main.cpp` is currently a demo/test harness, not the final app runtime.
- `frontend/` is a placeholder. The planned frontend is React + TypeScript.
- `scripts/generate_courses_csv.py` fetches UCSB Coursedog data and writes the CSV.

Target data flow:

```text
UCSB Coursedog catalog
  -> scripts/generate_courses_csv.py
  -> backend/data/courses.csv
  -> PostgreSQL import
  -> C++ graph/query layer
  -> API
  -> React/TypeScript visualization
```

## Global Agent Rules

- Read the relevant code before editing. This project is small enough that most backend/data tasks should inspect the touched files directly.
- Do not read, print, commit, or summarize secret values from `.env`.
- Do not run destructive database commands such as `docker compose down -v` unless the user explicitly asks for a reset.
- Do not rewrite generated or data-heavy files unless the task is specifically about data generation/import.
- Keep changes scoped to the assigned lane. If multiple agents are working, do not edit another lane's owned files without coordinating.
- Prefer small, reviewable patches over broad refactors.
- Add or update tests for behavior changes whenever a test surface exists. If tests do not exist yet, create the smallest useful harness for the lane.
- Keep documentation and code names honest. For example, `CountPaths` currently returns shortest path length, not a count of all paths.

## Rate-Limit Pause And Handoff

If Codex usage, credit, or rate-limit remaining reaches 10% or lower, all agents must pause project work and preserve progress for after limits reset.

Trigger conditions:

- The Codex Usage panel, `/status`, CLI/app warning, or parent coordinator reports 10% or less remaining usage/credits.
- A rate-limit or credit exhaustion warning appears and continuing could strand partial work.
- A `429`, `Too Many Requests`, or `rate limit reached` response occurs during agent work.

Required behavior:

- Stop spawning or messaging subagents.
- Do not start new implementation work, broad searches, dependency installs, builds, test suites, generated-data refreshes, or Docker/database operations.
- Finish only the smallest safe atomic action already in progress, such as completing the current file write if stopping mid-write would corrupt a file.
- Do not repeatedly retry rate-limited requests. Back off and wait for the reset instead of burning remaining quota.
- Avoid committing or pushing automatically unless the user explicitly asked for that before the pause.

Progress preservation steps:

1. Run `git status --short` if possible.
2. Capture the active lane, current task, files changed, commands already run, verification state, blockers, and exact next step.
3. If enough quota remains for one small write, create or update `AGENT_HANDOFF.md` at the repo root with the handoff template below. This file is a temporary working note for resuming after reset.
4. If there is not enough quota for a file write, put the same handoff content in the final response.
5. Tell the user work is paused because the remaining Codex limit is at or below 10%, and recommend resuming the same thread with `/resume` after reset. If the thread is large, recommend `/compact` before resuming.

Handoff template:

```md
# Agent Handoff

Paused because Codex remaining usage was at or below 10%.

## Active Lane

- Lane:
- Task:
- Status:

## Files Touched

- 

## Commands Run

- 

## Verification

- Passed:
- Not run:
- Blocked:

## Current State

- 

## Next Step After Limit Reset

1. 
```

## Current Tooling And Commands

Use PowerShell-compatible commands on Windows.

Build C++ backend:

```powershell
mingw32-make
```

Run current demo executable:

```powershell
.\build\requisite-visualization.exe
```

Clean build artifacts:

```powershell
mingw32-make clean
```

Start local PostgreSQL:

```powershell
docker compose up -d postgres
```

If `docker` is installed but not on PATH, use:

```powershell
.\scripts\enable-docker-path.ps1
```

Check DB connectivity and seed state:

```powershell
.\scripts\test-db-connection.ps1
```

Regenerate course CSV from UCSB Coursedog:

```powershell
python .\scripts\generate_courses_csv.py --output backend\data\courses.csv
```

Notes:

- `make` may not exist on Windows; `mingw32-make` is known to work in the current environment.
- Python dependencies are not pinned yet. `scripts/generate_courses_csv.py` requires `requests`.
- Docker Desktop may be installed at `C:\Program Files\Docker\Docker\resources\bin\docker.exe` even when `docker` is not on PATH.

## Architecture Notes

Important files:

- `backend/include/Course.h`: current course model.
- `backend/include/Graph.h`: graph API and `PrereqGroups`.
- `backend/src/Graph.cpp`: CSV parsing, prerequisite parsing, graph construction, and BFS traversal are currently mixed together.
- `backend/src/main.cpp`: demo executable with hardcoded path checks.
- `backend/src/DatabaseConfig.cpp`: reads DB config but does not connect to PostgreSQL.
- `backend/db/migrations/001_initial_schema.sql`: schema for courses and grouped prerequisites.
- `backend/db/seeds/001_sample_data.sql`: small sample dataset only.
- `backend/data/courses.csv`: generated CoE catalog data.
- `scripts/generate_courses_csv.py`: UCSB catalog fetch and prerequisite parser.
- `frontend/.gitkeep`: placeholder for the future React frontend.

Known project facts from the planning pass:

- `backend/data/courses.csv` has 1,145 College of Engineering course rows.
- 351 rows have prerequisites.
- 144 rows contain OR prerequisite groups.
- `TMP 492` and `TMP 493` have blank credits.
- 91 prerequisite references point outside the current CoE CSV catalog.
- The Postgres seed has only 8 sample courses, 8 groups, and 13 options.

## Parallel Work Lanes

Use these lanes to split work across agents. Each lane owns a distinct set of files and should avoid overlapping edits with other lanes.

### Lane 1: Backend Graph And Catalog

Primary ownership:

- `backend/include/Graph.h`
- `backend/src/Graph.cpp`
- new backend catalog/parser files under `backend/include/` and `backend/src/`

Near-term goals:

- Split CSV parsing and prerequisite parsing out of `Graph.cpp`.
- Introduce a catalog abstraction that loads course data once.
- Rename or replace `CountPaths` with a correctly named shortest-path API.
- Add graph queries needed by visualization: direct prerequisites, recursive prerequisites, direct dependents, recursive dependents, shortest path with path reconstruction, and cycle detection.
- Preserve grouped prerequisite semantics separately from flattened graph edges.

Implementation guidance:

- Keep flattened edges for traversal, but do not lose `all` vs `any` prerequisite groups.
- Prefer const references for string inputs.
- Keep heavy includes out of headers when possible.
- Do not make PostgreSQL mandatory for this lane until the DB import/API work is ready.

Suggested verification:

```powershell
mingw32-make
.\build\requisite-visualization.exe
```

### Lane 2: Database And Import Pipeline

Primary ownership:

- `backend/db/migrations/`
- `backend/db/seeds/`
- new DB import scripts under `scripts/`
- DB-related README sections when needed

Near-term goals:

- Load the full `backend/data/courses.csv` catalog into PostgreSQL.
- Parse CSV prerequisites into `course_prerequisite_groups` and `course_prerequisite_options`.
- Decide and document how external prerequisite references are represented.
- Fix credits modeling for blank, variable, or nonstandard credit values.
- Separate sample seed data from real catalog import data.

Implementation guidance:

- Treat `001_sample_data.sql` as sample data, not the production catalog.
- Do not force `prerequisite_id` to reference `courses(id)` until external prerequisite handling is settled.
- Preserve group order and option order during import.
- Prefer idempotent imports for local development.

Suggested verification:

```powershell
docker compose up -d postgres
.\scripts\test-db-connection.ps1
```

Add focused SQL checks for:

- total course count
- prerequisite group count
- prerequisite option count
- sample courses with OR groups
- external prerequisite references

### Lane 3: API And Integration Boundary

Primary ownership:

- new API files under `backend/`
- `backend/src/DatabaseConfig.cpp`
- `backend/include/DatabaseConfig.h`
- build files needed to compile/link API dependencies

Near-term goals:

- Choose and document the API strategy before large implementation.
- Define JSON response contracts for courses, prerequisite groups, graph nodes, graph edges, and paths.
- Add endpoints for course lookup, search, prerequisites, dependents, graph neighborhoods, and paths.
- Connect API queries to either the in-memory catalog or PostgreSQL-backed catalog.

Suggested endpoint shape:

```text
GET /courses
GET /courses/:id
GET /courses/:id/prerequisites
GET /courses/:id/dependents
GET /graph?course=CMPSC%2016&direction=both&depth=3
GET /paths?from=CMPSC%208&to=CMPSC%20189A
```

Implementation guidance:

- Keep API contracts stable and small before building the frontend.
- Redact passwords in logs.
- Validate DB config instead of silently accepting invalid values.
- If adding third-party C++ dependencies, document installation and Windows build steps.

### Lane 4: Frontend Visualization

Primary ownership:

- `frontend/`
- frontend-related Docker/README additions

Near-term goals:

- Scaffold React + TypeScript, preferably with Vite.
- Build course search and selected-course detail views.
- Render prerequisite/dependent graph data from mocked JSON first, then API.
- Visually distinguish required prerequisites from alternative prerequisite groups.
- Add basic controls: graph depth, direction, subject filter, and fit-to-view.

Implementation guidance:

- Do not build a marketing landing page. The first screen should be the usable course explorer.
- Prefer established graph libraries over hand-rolled graph layout.
- Consider Cytoscape.js for dense graph exploration or React Flow for a node-board style UI.
- Keep visual styling quiet and utilitarian: this is a planning/exploration tool, not a promotional site.

Suggested verification after frontend exists:

```powershell
npm install
npm run dev
npm run build
```

Exact commands may change after the frontend is scaffolded; update this file when they do.

### Lane 5: Testing And Dev Experience

Primary ownership:

- new test files
- `Makefile`
- `.github/workflows/`
- Python dependency files such as `requirements.txt` or `pyproject.toml`
- helper scripts under `scripts/`

Near-term goals:

- Move hardcoded checks out of `backend/src/main.cpp` into tests.
- Add C++ tests for CSV parsing, prerequisite parsing, and graph traversal.
- Add Python fixture tests for `scripts/generate_courses_csv.py`.
- Add DB integration checks for schema/import behavior.
- Document reliable Windows build/test commands.
- Add CI once commands are stable.

Implementation guidance:

- Keep the first test framework lightweight. `doctest` or Catch2 is reasonable for C++.
- Test tricky prerequisite cases: OR groups, semicolon groups, W courses, course ranges, concurrent enrollment text, and non-course requirements.
- Avoid requiring network access for parser unit tests; use fixtures.

### Lane 6: Documentation And Product Decisions

Primary ownership:

- `README.md`
- `feedback.md`
- new files under `docs/`
- this `AGENTS.md`

Near-term goals:

- Replace README placeholders with a clear roadmap.
- Fix wording such as "perquisites" -> "prerequisites".
- Document project scope, data flow, setup commands, and current limitations.
- Add `docs/architecture.md` for the system design.
- Add `docs/data-quality.md` for parser limitations and catalog data caveats.
- Capture open decisions so implementation agents do not guess silently.

Open decisions to document:

- UCSB CoE only vs all UCSB departments.
- PostgreSQL as source of truth vs CSV as runtime source.
- C++ API vs a web-native backend.
- How to model external prerequisites.
- How to model concurrent enrollment, minimum grades, standing requirements, and instructor consent.
- Whether this is a local-only tool, deployed web app, class project, or portfolio app.

## Coordination Rules For Multiple Agents

- Start by choosing one lane and stating the files you expect to touch.
- If a task crosses lanes, identify the handoff point and make the smallest shared contract first.
- Do not let two agents edit the same file at the same time unless one is explicitly integrating.
- Backend graph work should not block on API/frontend decisions; keep a clean in-memory interface.
- Frontend work should use mocked API responses until API contracts are stable.
- DB import work should not rewrite the C++ parser unless Lane 1 agrees on shared parsing ownership.
- Documentation updates should reflect actual behavior, not planned behavior, unless clearly labeled as roadmap.

## Definition Of Done

For any code change:

- Relevant build/test/check commands were run, or the reason they could not run is documented.
- New behavior is covered by tests when practical.
- Public names match behavior.
- Secrets are not printed or committed.
- README/AGENTS/docs are updated when setup commands, architecture, or workflow changes.
- The final response states what changed, what was verified, and what remains.
