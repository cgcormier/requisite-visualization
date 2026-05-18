import { ArrowDownToLine, ArrowUpFromLine, BookOpen } from 'lucide-react';
import type { CourseDetail as CourseDetailType, CourseRelationshipResponse } from '../types';

interface CourseDetailProps {
  course: CourseDetailType;
  prerequisiteResponse: CourseRelationshipResponse;
  dependentResponse: CourseRelationshipResponse;
  onSelectCourse: (courseId: string) => void;
}

function CourseDetail({
  course,
  prerequisiteResponse,
  dependentResponse,
  onSelectCourse,
}: CourseDetailProps) {
  return (
    <aside className="detail-panel" aria-label="Selected course detail">
      <div className="detail-heading">
        <BookOpen aria-hidden="true" size={20} />
        <div>
          <p className="detail-kicker">Selected course</p>
          <h2>{course.id}</h2>
        </div>
      </div>

      <div className="detail-summary">
        <h3>{course.name}</h3>
        <dl>
          <div>
            <dt>Credits</dt>
            <dd>{course.credits ?? 'Variable'}</dd>
          </div>
          <div>
            <dt>College</dt>
            <dd>{course.college}</dd>
          </div>
        </dl>
      </div>

      <RelationshipSection
        icon={<ArrowDownToLine aria-hidden="true" size={18} />}
        title="Prerequisites"
        response={prerequisiteResponse}
        emptyText="No prerequisites in the mock catalog."
        onSelectCourse={onSelectCourse}
      />

      <RelationshipSection
        icon={<ArrowUpFromLine aria-hidden="true" size={18} />}
        title="Dependents"
        response={dependentResponse}
        emptyText="No dependents in the mock catalog."
        onSelectCourse={onSelectCourse}
      />
    </aside>
  );
}

interface RelationshipSectionProps {
  icon: React.ReactNode;
  title: string;
  response: CourseRelationshipResponse;
  emptyText: string;
  onSelectCourse: (courseId: string) => void;
}

function RelationshipSection({
  icon,
  title,
  response,
  emptyText,
  onSelectCourse,
}: RelationshipSectionProps) {
  return (
    <section className="relationship-section">
      <div className="section-title">
        {icon}
        <h3>{title}</h3>
      </div>

      {response.groups.length === 0 ? (
        <p className="empty-state">{emptyText}</p>
      ) : (
        <div className="group-stack">
          {response.groups.map((group, groupIndex) => (
            <div className="prereq-group" key={`${title}-${groupIndex}`}>
              <span className={`group-label ${group.type}`}>
                {group.type === 'all' ? 'Required group' : 'Alternative group'}
              </span>
              <div className="option-row">
                {group.options.map((option) => (
                  <button
                    className="course-chip"
                    key={`${title}-${groupIndex}-${option.courseId}`}
                    type="button"
                    disabled={option.external}
                    onClick={() => onSelectCourse(option.courseId)}
                    title={option.external ? 'External prerequisite' : `Select ${option.courseId}`}
                  >
                    {option.courseId}
                    {option.external ? <span>External</span> : null}
                  </button>
                ))}
              </div>
            </div>
          ))}
        </div>
      )}
    </section>
  );
}

export default CourseDetail;
