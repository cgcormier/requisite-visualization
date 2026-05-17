#!/usr/bin/env python3
"""Generate courses.csv from UCSB's public Coursedog catalog feed."""

from __future__ import annotations

import argparse
import csv
import html
import re
import sys
from collections import Counter
from pathlib import Path
from typing import Iterable

import requests


CATALOG_ID = "mZXlGvYb30h2fSq3aYLn"
CATALOG_URL = (
    "https://app.coursedog.com/api/v1/cm/ucsb/courses/search/%24filters"
    f"?catalogId={CATALOG_ID}"
    "&skip=0"
    "&limit=50000"
    "&orderBy=code"
    "&formatDependents=false"
    "&effectiveDatesRange=2026-03-30%2C2026-03-30"
    "&ignoreEffectiveDating=false"
    "&columns=code%2CcourseGroupId%2CsubjectCode%2CcourseNumber%2ClongName%2Ccredits%2Ccollege%2Crequisites"
)

HEADERS = {
    "Accept": "application/json, text/plain, */*",
    "Content-Type": "application/json",
    "Origin": "https://catalog.ucsb.edu",
    "Referer": "https://catalog.ucsb.edu/courses",
    "User-Agent": "Mozilla/5.0",
}

FILTERS = {
    "filters": [
        {
            "id": "description-course",
            "condition": "field",
            "name": "description",
            "inputType": "text",
            "group": "course",
            "type": "isNotEmpty",
        },
        {
            "id": "startTerm-course",
            "condition": "field",
            "name": "startTerm",
            "inputType": "text",
            "group": "course",
            "type": "isNotEmpty",
        },
        {
            "id": "HiPz3-course",
            "condition": "field",
            "name": "HiPz3",
            "inputType": "boolean",
            "group": "course",
            "type": "isNot",
            "value": True,
            "customField": True,
        },
    ],
    "id": "HXkaROAK",
    "condition": "and",
}

NAME_MAP = [
    (r"\bElectrical\s+and\s+Computer\s+Engineering\b", "ECE"),
    (r"\bElectrical\s+Engineering\b", "ECE"),
    (r"\bComputer\s*Science\b", "CMPSC"),
    (r"\bComp\.?\s*Sci\.?\b", "CMPSC"),
    (r"\bChemical\s+Engineering\b", "CH E"),
    (r"\bChemical\s+Engr\b", "CH E"),
    (r"\bMechanical\s+Engineering\b", "ME"),
    (r"\bMechanical\s+Engr\b", "ME"),
    (r"\bEngineering\b", "ENGR"),
    (r"\bEngr\b", "ENGR"),
    (r"\bMathematics\b", "MATH"),
    (r"\bMath\b", "MATH"),
    (r"\bChemistry\b", "CHEM"),
    (r"\bChem\b", "CHEM"),
    (r"\bPhysics\b", "PHYS"),
    (r"\bPhys\b", "PHYS"),
    (r"\bStatistics\s+and\s+Applied\s+Probability\b", "PSTAT"),
    (r"\bStatistics\b", "PSTAT"),
    (r"\bMaterials\b", "MATRL"),
    (r"\bBioengineering\b", "BIOE"),
    (
        r"\bMolecular\s*,?\s*Cellular\s*(?:and|&)\s*Developmental\s*Biology\b",
        "MCDB",
    ),
    (r"\bWriting\b", "WRIT"),
    (r"\bCS\b", "CMPSC"),
    (r"\bEE\b", "ECE"),
]


def fetch_catalog_records() -> list[dict]:
    response = requests.post(CATALOG_URL, json=FILTERS, headers=HEADERS, timeout=120)
    response.raise_for_status()
    return response.json()["data"]


def normalize_subject(subject: str) -> str:
    subject = " ".join(subject.upper().split())
    if subject == "CMPSCW":
        return "CMPSC W"
    return subject


def course_id(subject: str, number: str) -> str:
    subject = normalize_subject(subject)
    number = number.upper().replace(" ", "")
    if subject.endswith(" W"):
        return f"{subject}{number}"
    return f"{subject} {number}"


def course_units(course: dict) -> str:
    credits = course.get("credits") or {}
    credit_hours = credits.get("creditHours") or {}
    value = credit_hours.get("min") or credits.get("numberOfCredits") or ""
    return str(value)


def raw_requisite_text(course: dict) -> str:
    requisites = course.get("requisites") or {}
    freeform = requisites.get("requisitesFreeform") or {}
    return freeform.get("value") or ""


def clean_text(value: str) -> str:
    if not value:
        return ""
    value = re.sub(r"<\s*(br|/p|/div|/li)\s*/?>", "; ", value, flags=re.I)
    value = re.sub(r"<[^>]+>", " ", value)
    value = html.unescape(value).replace("\xa0", " ")
    value = value.replace("\u2013", "-").replace("\u2014", "-")
    value = value.replace("\u2019", "'")
    return re.sub(r"\s+", " ", value).strip()


def standardize_subject_names(text: str) -> str:
    for pattern, replacement in NAME_MAP:
        text = re.sub(pattern, replacement, text, flags=re.I)
    return text


def strip_non_course_requirements(text: str) -> str:
    text = re.sub(r"(?i)\bpre[- ]?requisites?\s*:\s*", "", text)
    text = re.sub(r"(?i)\(\s*may be taken concurrently(?:[^)]*)\)", " ", text)
    text = re.sub(
        r"(?i)\bmay be taken concurrently(?:\s+with\s+[A-Z ]+\d+[A-Z]*)?",
        " ",
        text,
    )
    text = re.sub(
        r"(?i)\bwith\s+(?:a\s+)?(?:minimum\s+)?(?:letter\s+)?grade\s+of\s+"
        r"[A-F][+-]?\s*(?:or\s+better)?(?:\s+in\s+each(?:\s+of\s+those\s+courses)?)?",
        " ",
        text,
    )
    text = re.sub(
        r"(?i)\bwith\s+minimum\s+grade\s+of\s+[A-F][+-]?\s*(?:or\s+better)?",
        " ",
        text,
    )
    text = re.sub(
        r"(?i)\bminimum\s+grade\s+of\s+[A-F][+-]?\s*(?:or\s+better)?",
        " ",
        text,
    )
    text = re.sub(r"(?i)\bmust\s+be\s+concurrently\s+enrolled\s+in\b", "", text)
    text = re.sub(
        r"(?i)(?:,?\s*or\s+)?(?:consent|permission)\s+of\s+(?:the\s+)?instructor",
        " ",
        text,
    )
    text = re.sub(r"(?i)(?:,?\s*or\s+)?instructor\s+(?:permission|consent)", " ", text)
    text = re.sub(
        r"(?i)(?:,?\s*or\s+)?significant\s+prior\s+programming\s+experience",
        " ",
        text,
    )
    text = re.sub(r"(?i)(?:,?\s*or\s+)?equivalent(?:\s+course(?:work)?)?", " ", text)
    text = re.sub(r"(?i)(?:,?\s*or\s+)?similar\s+course(?:work)?", " ", text)
    text = re.sub(r"(?i)\bopen\s+to\s+[^.;]*?(?:majors?|students)\s+only\b", " ", text)
    text = re.sub(r"(?i)\bnot\s+open\s+for\s+credit\s+[^.;]*", " ", text)
    text = re.sub(r"(?i)\bfor\s+undergraduates\s*,?", " ", text)
    text = re.sub(r"(?i)\b(?:upper[- ]division|graduate|senior|junior)\s+standing\b", " ", text)
    text = re.sub(r"(?i)\bcompletion\s+of\s+\d+\s+upper[- ]division\s+courses\s+in\s+[^.;]*", " ", text)
    text = re.sub(r"(?i)\badmission\s+to\s+[^.;]*", " ", text)
    return re.sub(r"\s+", " ", text).strip(" ;,.")


def normalize_matched_subject(value: str, subjects: Iterable[str]) -> str:
    compact = value.upper().replace(" ", "")
    for subject in subjects:
        normalized = normalize_subject(subject)
        if compact == normalized.replace(" ", ""):
            return normalized
    return normalize_subject(value)


def expand_number(number: str) -> list[str]:
    number = number.upper().replace(" ", "")
    parts = [part for part in re.split(r"-+", number) if part]
    if len(parts) == 1:
        return parts

    first = parts[0]
    match = re.match(r"^(\d+)([A-Z]*)$", first)
    if not match:
        return parts

    base, first_suffix = match.groups()
    if (
        len(parts) == 2
        and len(first_suffix) == 1
        and re.fullmatch(r"[A-Z]", parts[1])
        and first_suffix <= parts[1]
    ):
        return [base + chr(code) for code in range(ord(first_suffix), ord(parts[1]) + 1)]

    expanded = [first]
    for part in parts[1:]:
        if re.match(r"^\d+", part):
            expanded.append(part)
        else:
            expanded.append(base + part)
    return expanded


def token_pattern(subjects: Iterable[str]) -> re.Pattern:
    variants = []
    for subject in subjects:
        normalized = normalize_subject(subject)
        if normalized:
            variants.append(re.escape(normalized).replace(r"\ ", r"\s*"))
            variants.append(re.escape(normalized.replace(" ", "")))
    for extra in ("CMPSC W", "CMPSCW", "CH E W", "CHEW", "ENGR W", "ENGRW", "ME W", "MEW"):
        variants.append(re.escape(extra).replace(r"\ ", r"\s*"))
    subject_re = "|".join(sorted(set(variants), key=len, reverse=True))
    return re.compile(
        rf"(?P<subj>{subject_re})\s*(?P<num>\d{{1,3}}[A-Z]{{0,3}}"
        rf"(?:\s*-\s*(?:\d{{1,3}})?[A-Z]{{1,3}})*)"
        rf"|(?P<bare>\b\d{{1,3}}[A-Z]{{0,3}}(?:\s*-\s*(?:\d{{1,3}})?[A-Z]{{1,3}})*\b)",
        re.I,
    )


def expand_bare_letter_refs(text: str, subjects: Iterable[str]) -> str:
    subject_re = "|".join(
        sorted(
            set(re.escape(normalize_subject(subject)).replace(r"\ ", r"\s*") for subject in subjects),
            key=len,
            reverse=True,
        )
    )
    if not subject_re:
        return text
    pattern = re.compile(
        rf"(?P<subj>{subject_re})\s+(?P<base>\d{{1,3}})(?P<suffix>[A-Z]{{1,3}})"
        rf"\s+(?P<conn>and|or)\s+(?!(?:{subject_re})\s*\d)"
        rf"(?P<bare>[A-Z]{{1,3}})\b",
        re.I,
    )
    previous = None
    iterations = 0
    while previous != text and iterations < 5:
        iterations += 1
        previous = text

        def replace_bare_suffix(match: re.Match) -> str:
            if not match.group("bare").isupper():
                return match.group(0)
            return (
                f"{match.group('subj')} {match.group('base')}{match.group('suffix')} "
                f"{match.group('conn')} {match.group('subj')} {match.group('base')}{match.group('bare')}"
            )

        text = pattern.sub(
            replace_bare_suffix,
            text,
        )
    return text


def dedupe(values: Iterable[str]) -> list[str]:
    seen = set()
    output = []
    for value in values:
        if value and value not in seen:
            seen.add(value)
            output.append(value)
    return output


def extract_courses(fragment: str, compiled_token: re.Pattern, subjects: Iterable[str]) -> list[str]:
    courses = []
    last_subject = ""
    for match in compiled_token.finditer(fragment):
        if match.group("subj"):
            last_subject = normalize_matched_subject(match.group("subj"), subjects)
            numbers = expand_number(match.group("num"))
        else:
            if not last_subject:
                continue
            before = fragment[max(0, match.start() - 35) : match.start()].lower()
            if re.search(r"(maximum|minimum|unit|units|completion of|coursework)\s+$", before):
                continue
            numbers = expand_number(match.group("bare"))

        courses.extend(course_id(last_subject, number) for number in numbers)
    return dedupe(courses)


def has_or(fragment: str) -> bool:
    return bool(re.search(r"(?i)\bor\b|/", fragment))


def remove_trailing(values: list[str], suffix: list[str]) -> bool:
    if suffix and values[-len(suffix) :] == suffix:
        del values[-len(suffix) :]
        return True
    return False


def parse_prereqs(raw: str, compiled_token: re.Pattern, subjects: Iterable[str]) -> tuple[list[str], list[list[str]], list[str]]:
    flags = []
    text = strip_non_course_requirements(standardize_subject_names(clean_text(raw)))
    if not text:
        return [], [], flags

    text = expand_bare_letter_refs(text, subjects)
    if re.search(r"(?i)\bat\s+least\s+one\b|\bamong\s+the\s+following\b|\bone\s+of\s+the\s+following\b", text):
        flags.append("at_least_one")
    if re.search(r"(?i)\bor\b", text) and re.search(r"(?i)\band\b", text) and "-" in text:
        flags.append("complex_bundle_or")

    and_prereqs: list[str] = []
    or_groups: list[list[str]] = []
    last_clause_courses: list[str] = []

    for raw_clause in re.split(r"[.;]", text):
        clause = raw_clause.strip(" ,")
        if not clause:
            continue

        leading_or = bool(re.match(r"(?i)^or\b", clause))
        clause = re.sub(r"(?i)^or\b\s*,?\s*", "", clause).strip()
        if not clause:
            continue

        marker = re.search(
            r"(?i)(?:at\s+least\s+one\s+(?:class\s+)?among\s+the\s+following|"
            r"one\s+of\s+the\s+following)\s*:",
            clause,
        )
        if marker:
            before = re.sub(r"(?i)\band\s*$", "", clause[: marker.start()]).strip(" ,")
            after = clause[marker.end() :].strip(" ,")
            before_courses = extract_courses(before, compiled_token, subjects)
            after_courses = extract_courses(after, compiled_token, subjects)
            if before_courses:
                if has_or(before):
                    or_groups.append(before_courses)
                else:
                    and_prereqs.extend(before_courses)
            if after_courses:
                or_groups.append(after_courses)
            last_clause_courses = before_courses + after_courses
            continue

        normalized_clause = re.sub(r"(?i),\s*or\s+", " or ", clause)
        parts = [normalized_clause]
        if has_or(normalized_clause):
            parts = [
                part.strip(" ,")
                for part in re.split(r"\s*,\s*|\s+\band\b\s+", normalized_clause, flags=re.I)
                if part.strip(" ,")
            ]

        clause_courses: list[str] = []
        for part in parts:
            part_courses = extract_courses(part, compiled_token, subjects)
            if not part_courses:
                continue

            if leading_or and last_clause_courses:
                removed = remove_trailing(and_prereqs, last_clause_courses)
                if removed:
                    flags.append("leading_or_bundle")
                or_groups.append(dedupe(last_clause_courses + part_courses))
                leading_or = False
            elif has_or(part):
                or_groups.append(part_courses)
            else:
                and_prereqs.extend(part_courses)
            clause_courses.extend(part_courses)

        if clause_courses:
            last_clause_courses = clause_courses

    return dedupe(and_prereqs), [group for group in (dedupe(group) for group in or_groups) if group], flags


def remove_self_reference(
    own_id: str, and_prereqs: list[str], or_groups: list[list[str]]
) -> tuple[list[str], list[list[str]]]:
    and_prereqs = [course for course in and_prereqs if course != own_id]
    or_groups = [[course for course in group if course != own_id] for group in or_groups]
    return and_prereqs, [group for group in or_groups if group]


def format_prereqs(and_prereqs: list[str], or_groups: list[list[str]]) -> str:
    parts = []
    if and_prereqs:
        parts.append(", ".join(and_prereqs))
    if or_groups:
        parts.append("| " + "; ".join(", ".join(group) for group in or_groups))
    return " ".join(parts)


def generate_rows(records: list[dict]) -> tuple[list[list[str]], Counter, Counter]:
    subjects = sorted(
        {normalize_subject(record.get("subjectCode") or "") for record in records if record.get("subjectCode")}
    )
    compiled_token = token_pattern(subjects)
    rows = []
    flag_counts: Counter = Counter()
    subject_counts: Counter = Counter()

    engineering_courses = [
        record for record in records if record.get("college") == "College of Engineering"
    ]
    engineering_courses.sort(key=lambda course: (normalize_subject(course.get("subjectCode") or ""), course.get("courseNumber") or ""))

    for course in engineering_courses:
        own_id = course_id(course.get("subjectCode") or "", course.get("courseNumber") or "")
        and_prereqs, or_groups, flags = parse_prereqs(raw_requisite_text(course), compiled_token, subjects)
        and_prereqs, or_groups = remove_self_reference(own_id, and_prereqs, or_groups)
        flag_counts.update(flags)
        subject_counts[normalize_subject(course.get("subjectCode") or "")] += 1
        rows.append(
            [
                own_id,
                course.get("longName") or "",
                course_units(course),
                course.get("college") or "",
                format_prereqs(and_prereqs, or_groups),
            ]
        )

    return rows, flag_counts, subject_counts


def write_courses_csv(rows: list[list[str]], output_path: Path) -> None:
    with output_path.open("w", newline="", encoding="utf-8") as output:
        writer = csv.writer(output)
        writer.writerow(["id", "name", "credits", "college", "prereqs"])
        writer.writerows(rows)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--output",
        default=str(Path("backend") / "data" / "courses.csv"),
        help="Path to write. Defaults to backend/data/courses.csv.",
    )
    args = parser.parse_args()

    records = fetch_catalog_records()
    rows, flag_counts, subject_counts = generate_rows(records)
    write_courses_csv(rows, Path(args.output))

    print(f"Fetched {len(records)} active catalog courses.")
    print(f"Wrote {len(rows)} College of Engineering courses to {args.output}.")
    print("Subjects: " + ", ".join(f"{subject}={count}" for subject, count in subject_counts.most_common()))
    if flag_counts:
        print("Prereq conversion notes: " + ", ".join(f"{name}={count}" for name, count in flag_counts.items()))
    return 0


if __name__ == "__main__":
    sys.exit(main())
