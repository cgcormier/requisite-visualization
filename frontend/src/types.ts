export type PrerequisiteGroupType = 'all' | 'any';
export type GraphDirection = 'prerequisites' | 'dependents' | 'both';

export interface CourseSummary {
  id: string;
  name: string;
  credits: number | null;
  college: string;
}

export interface PrerequisiteOption {
  courseId: string;
  external: boolean;
}

export interface PrerequisiteGroup {
  type: PrerequisiteGroupType;
  options: PrerequisiteOption[];
}

export interface CourseDetail extends CourseSummary {
  prerequisiteGroups: PrerequisiteGroup[];
}

export interface CourseRelationshipResponse {
  courseId: string;
  groups: PrerequisiteGroup[];
  flattenedCourseIds: string[];
}

export interface GraphNode {
  id: string;
  label: string;
  name?: string;
  external: boolean;
}

export interface GraphEdge {
  from: string;
  to: string;
  relationship: 'prerequisite' | 'dependent';
  groupType: PrerequisiteGroupType;
  groupIndex: number;
}

export interface GraphResponse {
  rootCourseId: string;
  direction: GraphDirection;
  depth: number;
  nodes: GraphNode[];
  edges: GraphEdge[];
}
