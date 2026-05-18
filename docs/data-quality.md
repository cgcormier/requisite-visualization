# Data Quality

This document records known catalog and parser caveats for `requisite-visualization`.

## Current Data Source

Course data comes from UCSB Coursedog through `scripts/generate_courses_csv.py`. The script writes `backend/data/courses.csv`, which is currently consumed by the C++ graph prototype.

The generated CSV is treated as generated data. It should not be rewritten unless the task is specifically about data generation or import.

## Known Catalog Facts

The current planning pass identified these facts about `backend/data/courses.csv`:

- The CSV has 1,145 College of Engineering course rows.
- 351 rows have prerequisites.
- 144 rows contain OR prerequisite groups.
- `TMP 492` and `TMP 493` have blank credits.
- 91 prerequisite references point outside the current College of Engineering CSV catalog.

These facts should be rechecked after regenerating the CSV because the upstream catalog can change.

## Prerequisite Groups

Prerequisites are modeled as grouped requirements:

```text
AND course, AND course | OR option, OR option; OR option, OR option
```

An `all` group means every option is required. An `any` group means one option is required. Flattened graph edges are useful for traversal, but they are lossy if displayed as the only prerequisite model.

For example, if a course accepts `CMPSC 8` or `CMPSC 16`, both may appear as graph edges, but the product must still show that they are alternatives.

## External References

Some prerequisite references point to courses outside the current College of Engineering CSV catalog. This can happen when engineering courses require math, physics, chemistry, writing, or other non-CoE courses.

Open modeling options:

- Keep external references as plain prerequisite IDs.
- Create placeholder course nodes for external references.
- Expand the catalog source beyond College of Engineering.
- Store external references in a separate table or field.

Until this decision is made, imports and APIs should preserve external prerequisite text and avoid silently dropping it.

## Credits

The current schema and code need to account for blank, variable, and nonstandard credit values. A single required integer is not enough for all catalog rows.

Possible future representation:

- `credits_min`
- `credits_max`
- `credits_label`
- raw source credit text

Documentation and UI should not assume every course has a single fixed integer credit value.

## Parser Limitations To Track

The prerequisite parser should be tested against cases such as:

- OR groups.
- Semicolon-separated groups.
- Course ranges.
- W course variants.
- Concurrent enrollment language.
- Minimum grade requirements.
- Class standing requirements.
- Instructor consent.
- Non-course requirements.

Some of these requirements may not be representable as course nodes. The product should preserve the raw prerequisite text or parser notes so students can see requirements that the graph cannot model.

## Verification Checks For Import Work

When the database import pipeline is implemented, focused checks should cover:

- Total imported course count.
- Prerequisite group count.
- Prerequisite option count.
- Courses with OR groups.
- External prerequisite references.
- Blank or nonstandard credits.
- A few known graph neighborhoods and path results.

These checks belong with the database/import and testing lanes. This document records what should be verified, not that the import pipeline already exists.
