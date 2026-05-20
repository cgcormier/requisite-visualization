import type {
  ApiErrorBody,
  CourseDetail,
  CourseRelationshipResponse,
  CourseSummary,
  GraphDirection,
  GraphResponse,
} from '../types';

const DEFAULT_API_BASE_URL = 'http://127.0.0.1:8080';

export const API_BASE_URL =
  (import.meta.env.VITE_API_BASE_URL as string | undefined)?.replace(/\/+$/, '') ?? DEFAULT_API_BASE_URL;

type QueryParamValue = string | number | string[] | undefined;

interface CourseListParams extends Record<string, QueryParamValue> {
  q?: string;
  subject?: string;
  colleges?: string[];
  limit?: number;
}

interface GraphParams {
  course: string;
  direction: GraphDirection;
  depth: number;
  subject?: string;
  colleges?: string[];
}

export class ApiError extends Error {
  code: string;
  status: number;

  constructor(message: string, code: string, status: number) {
    super(message);
    this.name = 'ApiError';
    this.code = code;
    this.status = status;
  }
}

export function isAbortError(error: unknown): boolean {
  return error instanceof Error && error.name === 'AbortError';
}

export async function listCourses(params: CourseListParams = {}, signal?: AbortSignal): Promise<CourseSummary[]> {
  const payload = await fetchJson<unknown>('/courses', params, signal);
  const courses = Array.isArray(payload)
    ? payload
    : isObject(payload) && Array.isArray(payload.courses)
      ? payload.courses
      : null;

  if (!courses) {
    throw new ApiError('Unexpected course list response.', 'invalid_response', 0);
  }

  return courses.map(normalizeCourseSummary);
}

export async function getCourse(courseId: string, signal?: AbortSignal): Promise<CourseDetail> {
  return normalizeCourseDetail(await fetchJson<CourseDetail>(`/courses/${encodeURIComponent(courseId)}`, {}, signal));
}

export async function getPrerequisites(
  courseId: string,
  signal?: AbortSignal,
): Promise<CourseRelationshipResponse> {
  return fetchJson<CourseRelationshipResponse>(`/courses/${encodeURIComponent(courseId)}/prerequisites`, {}, signal);
}

export async function getDependents(courseId: string, signal?: AbortSignal): Promise<CourseRelationshipResponse> {
  return fetchJson<CourseRelationshipResponse>(`/courses/${encodeURIComponent(courseId)}/dependents`, {}, signal);
}

export async function getGraph(params: GraphParams, signal?: AbortSignal): Promise<GraphResponse> {
  return fetchJson<GraphResponse>(
    '/graph',
    {
      course: params.course,
      direction: params.direction,
      depth: params.depth,
      subject: params.subject,
      colleges: params.colleges,
    },
    signal,
  );
}

async function fetchJson<T>(
  path: string,
  params: Record<string, QueryParamValue>,
  signal?: AbortSignal,
): Promise<T> {
  const url = new URL(path, `${API_BASE_URL}/`);

  Object.entries(params).forEach(([key, value]) => {
    if (Array.isArray(value)) {
      if (value.length > 0) {
        url.searchParams.set(key, value.join(','));
      }
    } else if (value !== undefined && value !== '') {
      url.searchParams.set(key, String(value));
    }
  });

  const response = await fetch(url, {
    headers: { Accept: 'application/json' },
    signal,
  });

  const payload = await parseJson(response);

  if (!response.ok) {
    const apiError = toApiError(payload);
    throw new ApiError(apiError.message, apiError.code, response.status);
  }

  return payload as T;
}

async function parseJson(response: Response): Promise<unknown> {
  const text = await response.text();

  if (!text) {
    return null;
  }

  try {
    return JSON.parse(text) as unknown;
  } catch {
    throw new ApiError('API returned invalid JSON.', 'invalid_json', response.status);
  }
}

function toApiError(payload: unknown): { code: string; message: string } {
  if (isApiErrorBody(payload)) {
    return payload.error;
  }

  return {
    code: 'request_failed',
    message: 'API request failed.',
  };
}

function isApiErrorBody(payload: unknown): payload is ApiErrorBody {
  return (
    isObject(payload) &&
    isObject(payload.error) &&
    typeof payload.error.code === 'string' &&
    typeof payload.error.message === 'string'
  );
}

function normalizeCourseDetail(course: CourseDetail): CourseDetail {
  return {
    ...normalizeCourseSummary(course),
    prerequisiteGroups: course.prerequisiteGroups ?? [],
  };
}

function normalizeCourseSummary(course: CourseSummary): CourseSummary {
  return {
    id: course.id,
    name: course.name,
    credits: course.credits ?? null,
    college: course.college,
    department: course.department ?? null,
    subject: course.subject || course.id.split(' ')[0] || course.id,
  };
}

function isObject(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null;
}
