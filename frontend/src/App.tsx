import { useMemo, useState } from 'react';
import { Filter, Search } from 'lucide-react';
import CourseDetail from './components/CourseDetail';
import CourseSearch from './components/CourseSearch';
import ExplorerControls from './components/ExplorerControls';
import GraphExplorer from './components/GraphExplorer';
import {
  buildGraphResponse,
  courses,
  filterCourses,
  getCourseById,
  getDependents,
  getPrerequisites,
  getSubjects,
} from './data/mockCatalog';
import type { GraphDirection } from './types';

const initialCourseId = 'CMPSC 16';

function App() {
  const [query, setQuery] = useState('');
  const [subject, setSubject] = useState('all');
  const [selectedCourseId, setSelectedCourseId] = useState(initialCourseId);
  const [direction, setDirection] = useState<GraphDirection>('both');
  const [depth, setDepth] = useState(2);
  const [fitSignal, setFitSignal] = useState(0);
  const subjects = useMemo(() => getSubjects(), []);

  const filteredCourses = useMemo(() => filterCourses(query, subject), [query, subject]);
  const selectedCourse = getCourseById(selectedCourseId) ?? courses[0];
  const graph = useMemo(
    () => buildGraphResponse(selectedCourse.id, direction, depth, subject),
    [selectedCourse.id, direction, depth, subject],
  );
  const prerequisiteResponse = useMemo(() => getPrerequisites(selectedCourse.id), [selectedCourse.id]);
  const dependentResponse = useMemo(() => getDependents(selectedCourse.id), [selectedCourse.id]);

  return (
    <main className="app-shell">
      <header className="top-bar">
        <div className="brand-block">
          <span className="eyebrow">UCSB CoE Mock Catalog</span>
          <h1>Course Explorer</h1>
        </div>
        <div className="catalog-status" aria-label="Catalog status">
          <span>{courses.length} courses</span>
          <span>Mock API contract</span>
        </div>
      </header>

      <section className="workspace" aria-label="Course explorer workspace">
        <aside className="course-panel" aria-label="Course search and results">
          <div className="panel-toolbar">
            <label className="search-field">
              <Search aria-hidden="true" size={18} />
              <span className="sr-only">Search courses</span>
              <input
                type="search"
                value={query}
                onChange={(event) => setQuery(event.target.value)}
                placeholder="Search by course or title"
              />
            </label>

            <label className="subject-filter">
              <Filter aria-hidden="true" size={17} />
              <span className="sr-only">Subject filter</span>
              <select value={subject} onChange={(event) => setSubject(event.target.value)}>
                <option value="all">All subjects</option>
                {subjects.map((subjectCode) => (
                  <option key={subjectCode} value={subjectCode}>
                    {subjectCode}
                  </option>
                ))}
              </select>
            </label>
          </div>

          <CourseSearch
            courses={filteredCourses}
            selectedCourseId={selectedCourse.id}
            onSelectCourse={setSelectedCourseId}
          />
        </aside>

        <section className="graph-panel" aria-label="Prerequisite graph">
          <ExplorerControls
            direction={direction}
            depth={depth}
            graphNodeCount={graph.nodes.length}
            graphEdgeCount={graph.edges.length}
            onDepthChange={setDepth}
            onDirectionChange={setDirection}
            onFit={() => setFitSignal((value) => value + 1)}
          />
          <GraphExplorer graph={graph} fitSignal={fitSignal} onSelectCourse={setSelectedCourseId} />
        </section>

        <CourseDetail
          course={selectedCourse}
          prerequisiteResponse={prerequisiteResponse}
          dependentResponse={dependentResponse}
          onSelectCourse={setSelectedCourseId}
        />
      </section>
    </main>
  );
}

export default App;
