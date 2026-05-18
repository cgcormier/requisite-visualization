# API Strategy

This lane owns the API and integration boundary. The first implementation should keep the JSON contracts stable before choosing or adding an HTTP server dependency.

## Runtime Approach

- Start with transport-independent C++ response models and serialization helpers under `backend/include/api/` and `backend/src/api/`.
- Serve data from the in-memory catalog/graph interface first, then add a PostgreSQL-backed adapter after the import pipeline settles.
- Keep PostgreSQL optional until the database import path and external prerequisite representation are stable.
- Defer HTTP dependency selection. Candidate libraries should be compared in a small follow-up note or patch before implementation, including Windows build steps, dependency installation, routing support, JSON support, and CORS behavior.
- Do not add a web server framework until the response contracts below are accepted by the frontend and graph lanes.

## Database Environment Precedence

`DatabaseConfig` supports both app-oriented `DB_*` names and Docker Compose-oriented `POSTGRES_*` names:

1. `DATABASE_URL` is the complete connection override and takes precedence in connection-string builders.
2. When `DATABASE_URL` is absent, `DB_*` variables take precedence over matching `POSTGRES_*` variables.
3. `POSTGRES_*` variables are fallbacks so local Docker Compose settings can drive the app without duplicate values.
4. Built-in local defaults are used only when neither naming style is set.

Mapped component variables:

| Config field | Preferred | Fallback | Default |
| --- | --- | --- | --- |
| host | `DB_HOST` | `POSTGRES_HOST` | `localhost` |
| port | `DB_PORT` | `POSTGRES_PORT` | `5432` |
| database | `DB_NAME` | `POSTGRES_DB` | `requisite_visualization` |
| user | `DB_USER` | `POSTGRES_USER` | `requisite_user` |
| password | `DB_PASSWORD` | `POSTGRES_PASSWORD` | empty |

Secrets must not be logged. Redacted connection strings may show usernames, hostnames, ports, and database names, but password values must be replaced with `****`.

## Initial Endpoints

The proposed first API surface is read-only:

```text
GET /courses
GET /courses/:id
GET /courses/:id/prerequisites
GET /courses/:id/dependents
GET /graph?course=CMPSC%2016&direction=both&depth=3
GET /paths?from=CMPSC%208&to=CMPSC%20189A
```

Query behavior:

- `/courses` supports optional `q`, `subject`, and `limit` query parameters.
- `:id` uses the normalized catalog id such as `CMPSC 16`.
- `direction` accepts `prerequisites`, `dependents`, or `both`.
- `depth` defaults to `1` and should have a conservative maximum.
- `/paths` returns the shortest known path first; all-path enumeration is out of scope for the initial API.

## JSON Contracts

Course summary:

```json
{
  "id": "CMPSC 16",
  "name": "Problem Solving With Computers I",
  "credits": 4,
  "college": "ENGR"
}
```

Course detail:

```json
{
  "id": "CMPSC 16",
  "name": "Problem Solving With Computers I",
  "credits": 4,
  "college": "ENGR",
  "prerequisiteGroups": [
    {
      "type": "all",
      "options": [
        { "courseId": "MATH 3A", "external": true }
      ]
    }
  ]
}
```

Prerequisite or dependent response:

```json
{
  "courseId": "CMPSC 16",
  "groups": [
    {
      "type": "any",
      "options": [
        { "courseId": "CMPSC 8", "external": false },
        { "courseId": "ECE 15", "external": false }
      ]
    }
  ],
  "flattenedCourseIds": ["CMPSC 8", "ECE 15"]
}
```

Graph response:

```json
{
  "rootCourseId": "CMPSC 16",
  "direction": "both",
  "depth": 3,
  "nodes": [
    { "id": "CMPSC 16", "label": "CMPSC 16", "external": false }
  ],
  "edges": [
    {
      "from": "CMPSC 8",
      "to": "CMPSC 16",
      "relationship": "prerequisite",
      "groupType": "any",
      "groupIndex": 0
    }
  ]
}
```

Path response:

```json
{
  "from": "CMPSC 8",
  "to": "CMPSC 189A",
  "reachable": true,
  "distance": 4,
  "courseIds": ["CMPSC 8", "CMPSC 16", "CMPSC 24", "CMPSC 130A", "CMPSC 189A"]
}
```

Error response:

```json
{
  "error": {
    "code": "course_not_found",
    "message": "Course not found."
  }
}
```

## Handoffs

- Lane 1 should provide stable in-memory queries for direct prerequisites, recursive prerequisites, dependents, graph neighborhoods, and shortest path reconstruction.
- Lane 2 should settle external prerequisite storage before the PostgreSQL-backed adapter becomes the source for grouped prerequisite responses.
- Lane 4 can mock the JSON contracts in this file before an HTTP server exists.
- Lane 5 should add focused tests for config validation and contract serialization once the first API code lands.
