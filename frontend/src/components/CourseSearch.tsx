import type { CourseSummary } from '../types';

interface CourseSearchProps {
  courses: CourseSummary[];
  selectedCourseId: string;
  onSelectCourse: (courseId: string) => void;
}

function CourseSearch({ courses, selectedCourseId, onSelectCourse }: CourseSearchProps) {
  return (
    <div className="course-results" aria-live="polite">
      <div className="result-count">
        <span>{courses.length}</span>
        <span>{courses.length === 1 ? 'match' : 'matches'}</span>
      </div>

      <div className="result-list">
        {courses.map((course) => (
          <button
            className={`course-result ${course.id === selectedCourseId ? 'is-selected' : ''}`}
            key={course.id}
            type="button"
            onClick={() => onSelectCourse(course.id)}
          >
            <span className="course-result-code">{course.id}</span>
            <span className="course-result-name">{course.name}</span>
            <span className="course-result-meta">
              {course.credits ?? 'Var'} units · {course.college}
            </span>
          </button>
        ))}
      </div>
    </div>
  );
}

export default CourseSearch;
