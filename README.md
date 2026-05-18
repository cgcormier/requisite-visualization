# requisite-visualization
Visualizes perquisites for classes. Helpful for deciding course journey through college. 

# TODO X

## Local PostgreSQL with Docker Compose

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

The next `docker compose up -d postgres` will recreate the database and rerun the init scripts.

Connect with `psql` inside the container:

```powershell
docker compose exec postgres sh -lc 'psql -U "$POSTGRES_USER" -d "$POSTGRES_DB"'
```

Schema and seed files:

- `backend/db/migrations/001_initial_schema.sql`
- `backend/db/seeds/001_sample_data.sql`

Prerequisites are modeled with grouped requirements that match the CSV format:

```text
AND course, AND course | OR option, OR option; OR option, OR option
```

Every `all` group must be completed. Every `any` group requires one of its options. For graph-only queries, `course_dependency_edges` provides a flattened prerequisite-to-course edge view.

The C++ backend lives in `backend/src` with headers in `backend/include`. Course data lives in `backend/data/courses.csv`; set `COURSES_CSV_PATH` to override the CSV path.

The app reads database settings from `DATABASE_URL` or the `DB_HOST`, `DB_PORT`, `DB_NAME`, `DB_USER`, and `DB_PASSWORD` environment variables via `DatabaseConfig::fromEnvironment()`. The current app only loads connection configuration; it does not require a system PostgreSQL client library to build.
