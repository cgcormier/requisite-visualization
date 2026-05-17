BEGIN;

-- Idempotent sample data only. Re-running this seed inserts missing rows and
-- does not overwrite existing local edits.
INSERT INTO courses (id, name, credits, college) VALUES
    ('CMPSC 5A', 'Introduction to Data Science 1', 4, 'College of Engineering'),
    ('CMPSC 5B', 'Introduction to Data Science 2', 4, 'College of Engineering'),
    ('CMPSC 8', 'Introduction to Computer Science', 4, 'College of Engineering'),
    ('CMPSC 16', 'Problem Solving with Computers I', 4, 'College of Engineering'),
    ('CMPSC 24', 'Problem Solving with Computers II', 4, 'College of Engineering'),
    ('CMPSC 32', 'Object Oriented Design and Implementation', 4, 'College of Engineering'),
    ('CMPSC 40', 'Foundations of Computer Science', 5, 'College of Engineering'),
    ('CMPSC 170', 'Advanced Topics Sample Course', 4, 'College of Engineering')
ON CONFLICT (id) DO NOTHING;

-- This models the CSV prereq format:
-- AND courses | OR group; OR group
-- Example: CMPSC 40, CMPSC 32 | MATH 8, PSTAT 120A; ECE 139, PSTAT 120B
INSERT INTO course_prerequisite_groups (course_id, group_position, group_kind) VALUES
    ('CMPSC 5B', 1, 'all'),
    ('CMPSC 16', 1, 'any'),
    ('CMPSC 24', 1, 'all'),
    ('CMPSC 32', 1, 'all'),
    ('CMPSC 40', 1, 'all'),
    ('CMPSC 170', 1, 'all'),
    ('CMPSC 170', 2, 'any'),
    ('CMPSC 170', 3, 'any')
ON CONFLICT (course_id, group_position) DO NOTHING;

INSERT INTO course_prerequisite_options (course_id, group_position, option_position, prerequisite_id) VALUES
    ('CMPSC 5B', 1, 1, 'CMPSC 5A'),
    ('CMPSC 16', 1, 1, 'CMPSC 8'),
    ('CMPSC 16', 1, 2, 'ENGR 3'),
    ('CMPSC 16', 1, 3, 'ECE 3'),
    ('CMPSC 24', 1, 1, 'CMPSC 16'),
    ('CMPSC 32', 1, 1, 'CMPSC 24'),
    ('CMPSC 40', 1, 1, 'CMPSC 16'),
    ('CMPSC 170', 1, 1, 'CMPSC 40'),
    ('CMPSC 170', 1, 2, 'CMPSC 32'),
    ('CMPSC 170', 2, 1, 'MATH 8'),
    ('CMPSC 170', 2, 2, 'PSTAT 120A'),
    ('CMPSC 170', 3, 1, 'ECE 139'),
    ('CMPSC 170', 3, 2, 'PSTAT 120B')
ON CONFLICT (course_id, group_position, option_position) DO NOTHING;

COMMIT;
