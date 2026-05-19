#include "api/CsvCatalog.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <optional>
#include <queue>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace api {
namespace {

std::string trim(const std::string& value) {
    std::size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
        ++start;
    }

    std::size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }

    return value.substr(start, end - start);
}

std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string toUpper(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    return value;
}

std::vector<std::string> parseCsvRow(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    bool inQuotes = false;

    for (std::size_t i = 0; i < line.size(); ++i) {
        const char current = line[i];

        if (inQuotes) {
            if (current == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    field.push_back('"');
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else {
                field.push_back(current);
            }
        } else if (current == '"') {
            inQuotes = true;
        } else if (current == ',') {
            fields.push_back(field);
            field.clear();
        } else {
            field.push_back(current);
        }
    }

    fields.push_back(field);
    return fields;
}

int columnIndex(
    const std::unordered_map<std::string, int>& headers,
    const std::string& name,
    int fallback
) {
    const auto found = headers.find(name);
    return found == headers.end() ? fallback : found->second;
}

std::string fieldAt(const std::vector<std::string>& fields, int index) {
    if (index < 0 || static_cast<std::size_t>(index) >= fields.size()) {
        return "";
    }
    return trim(fields[static_cast<std::size_t>(index)]);
}

std::optional<double> parseCredits(const std::string& value) {
    const std::string trimmed = trim(value);
    if (trimmed.empty()) {
        return std::nullopt;
    }

    char* end = nullptr;
    const double parsed = std::strtod(trimmed.c_str(), &end);
    if (end == trimmed.c_str()) {
        return std::nullopt;
    }

    while (*end != '\0') {
        if (!std::isspace(static_cast<unsigned char>(*end))) {
            return std::nullopt;
        }
        ++end;
    }

    return parsed;
}

bool containsCaseInsensitive(const std::string& haystack, const std::string& needle) {
    return toLower(haystack).find(toLower(needle)) != std::string::npos;
}

void appendUnique(std::vector<std::string>& values, std::unordered_set<std::string>& seen, const std::string& value) {
    if (seen.insert(value).second) {
        values.push_back(value);
    }
}

} // namespace

bool CsvCatalog::load(std::string* errorMessage) {
    const char* overridePath = std::getenv("COURSES_CSV_PATH");
    const std::vector<std::string> candidatePaths = {
        overridePath == nullptr ? "" : overridePath,
        "backend/data/courses.csv",
        "data/courses.csv",
        "courses.csv",
    };

    for (const std::string& path : candidatePaths) {
        if (path.empty()) {
            continue;
        }

        std::ifstream file(path);
        if (file.is_open()) {
            file.close();
            return loadFromFile(path, errorMessage);
        }
    }

    if (errorMessage != nullptr) {
        *errorMessage = "Unable to open courses CSV file.";
    }
    return false;
}

bool CsvCatalog::loadFromFile(const std::string& path, std::string* errorMessage) {
    std::ifstream file(path);
    if (!file.is_open()) {
        if (errorMessage != nullptr) {
            *errorMessage = "Unable to open courses CSV file.";
        }
        return false;
    }

    std::string headerLine;
    if (!std::getline(file, headerLine)) {
        if (errorMessage != nullptr) {
            *errorMessage = "Courses CSV file is empty.";
        }
        return false;
    }

    const std::vector<std::string> headerFields = parseCsvRow(headerLine);
    std::unordered_map<std::string, int> headers;
    for (std::size_t i = 0; i < headerFields.size(); ++i) {
        headers[toLower(trim(headerFields[i]))] = static_cast<int>(i);
    }

    const int idIndex = columnIndex(headers, "id", 0);
    const int nameIndex = columnIndex(headers, "name", 1);
    const int creditsIndex = columnIndex(headers, "credits", 2);
    const int collegeIndex = columnIndex(headers, "college", 3);
    const int prereqsIndex = columnIndex(headers, "prereqs", 4);
    const int subjectIndex = columnIndex(headers, "subject", -1);
    const int departmentIndex = columnIndex(headers, "department", -1);

    std::vector<std::string> newCourseOrder;
    std::unordered_map<std::string, StoredCourse> newCourses;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }

        const std::vector<std::string> fields = parseCsvRow(line);
        const std::string id = normalizeCourseId(fieldAt(fields, idIndex));
        if (id.empty() || toLower(id) == "id") {
            continue;
        }

        CourseRecord course;
        course.id = id;
        course.name = fieldAt(fields, nameIndex);
        course.credits = parseCredits(fieldAt(fields, creditsIndex));
        course.college = fieldAt(fields, collegeIndex);
        course.department = fieldAt(fields, departmentIndex);
        course.subject = normalizeSubject(fieldAt(fields, subjectIndex));
        if (course.subject.empty()) {
            course.subject = deriveSubject(course.id);
        }

        StoredCourse stored;
        stored.course = course;
        stored.prerequisites = ParsePrereqGroups(fieldAt(fields, prereqsIndex));

        if (newCourses.find(course.id) == newCourses.end()) {
            newCourseOrder.push_back(course.id);
        }
        newCourses[course.id] = stored;
    }

    sourcePath_ = path;
    courseOrder_ = newCourseOrder;
    courses_ = newCourses;
    rebuildDependents();
    return true;
}

const std::string& CsvCatalog::sourcePath() const {
    return sourcePath_;
}

std::size_t CsvCatalog::size() const {
    return courses_.size();
}

const CourseRecord* CsvCatalog::findCourse(const std::string& id) const {
    const auto found = courses_.find(normalizeCourseId(id));
    return found == courses_.end() ? nullptr : &found->second.course;
}

bool CsvCatalog::containsCourse(const std::string& id) const {
    return findCourse(id) != nullptr;
}

std::vector<CourseRecord> CsvCatalog::searchCourses(const CourseSearchFilters& filters) const {
    std::vector<CourseRecord> results;
    if (filters.limit == 0) {
        return results;
    }

    results.reserve(std::min(filters.limit, courseOrder_.size()));

    for (const std::string& id : courseOrder_) {
        const auto found = courses_.find(id);
        if (found == courses_.end()) {
            continue;
        }

        if (!courseMatchesFilters(found->second.course, filters)) {
            continue;
        }

        results.push_back(found->second.course);
        if (results.size() >= filters.limit) {
            break;
        }
    }

    return results;
}

std::vector<PrerequisiteGroup> CsvCatalog::prerequisiteGroupsFor(const std::string& id) const {
    const auto found = courses_.find(normalizeCourseId(id));
    if (found == courses_.end()) {
        return {};
    }

    std::vector<PrerequisiteGroup> groups;
    const PrereqGroups& parsed = found->second.prerequisites;

    if (!parsed.andPrereqs.empty()) {
        PrerequisiteGroup group;
        group.groupType = "all";
        group.groupIndex = 0;
        for (const std::string& prereqId : parsed.andPrereqs) {
            const std::string normalizedId = normalizeCourseId(prereqId);
            if (!normalizedId.empty()) {
                group.options.push_back({normalizedId, !containsCourse(normalizedId)});
            }
        }
        if (!group.options.empty()) {
            groups.push_back(group);
        }
    }

    for (std::size_t i = 0; i < parsed.orGroups.size(); ++i) {
        PrerequisiteGroup group;
        group.groupType = "any";
        group.groupIndex = static_cast<int>(i);

        for (const std::string& prereqId : parsed.orGroups[i]) {
            const std::string normalizedId = normalizeCourseId(prereqId);
            if (!normalizedId.empty()) {
                group.options.push_back({normalizedId, !containsCourse(normalizedId)});
            }
        }

        if (!group.options.empty()) {
            groups.push_back(group);
        }
    }

    return groups;
}

std::vector<std::string> CsvCatalog::flattenedPrerequisitesFor(const std::string& id) const {
    std::vector<std::string> flattened;
    std::unordered_set<std::string> seen;

    for (const PrerequisiteGroup& group : prerequisiteGroupsFor(id)) {
        for (const PrerequisiteOption& option : group.options) {
            appendUnique(flattened, seen, option.courseId);
        }
    }

    return flattened;
}

std::vector<DependentRelationship> CsvCatalog::dependentsFor(const std::string& id) const {
    const auto found = dependentsByPrerequisite_.find(normalizeCourseId(id));
    return found == dependentsByPrerequisite_.end() ? std::vector<DependentRelationship>{} : found->second;
}

GraphResult CsvCatalog::graphFor(
    const std::string& rootCourseId,
    const std::string& direction,
    int depth,
    const std::unordered_set<std::string>& subjects,
    const std::unordered_set<std::string>& colleges
) const {
    GraphResult graph;
    graph.rootCourseId = normalizeCourseId(rootCourseId);
    graph.direction = direction;
    graph.depth = depth;

    std::queue<std::pair<std::string, int>> pending;
    std::unordered_map<std::string, int> distances;
    std::unordered_set<std::string> nodesAdded;
    std::unordered_set<std::string> edgesAdded;

    const auto addNode = [&](const std::string& courseId) {
        if (nodesAdded.insert(courseId).second) {
            graph.nodes.push_back(nodeFor(courseId));
        }
    };

    const auto addEdge = [&](const GraphEdge& edge) {
        const std::string key = edge.from + "\n" + edge.to + "\n" + edge.groupType + "\n" + std::to_string(edge.groupIndex);
        if (edgesAdded.insert(key).second) {
            graph.edges.push_back(edge);
        }
    };

    addNode(graph.rootCourseId);
    pending.push({graph.rootCourseId, 0});
    distances[graph.rootCourseId] = 0;

    while (!pending.empty()) {
        const std::string current = pending.front().first;
        const int currentDepth = pending.front().second;
        pending.pop();

        if (currentDepth >= depth) {
            continue;
        }

        if (direction == "prerequisites" || direction == "both") {
            for (const PrerequisiteGroup& group : prerequisiteGroupsFor(current)) {
                for (const PrerequisiteOption& option : group.options) {
                    if (!graphNodeAllowed(option.courseId, false, subjects, colleges)) {
                        continue;
                    }

                    addNode(option.courseId);
                    addEdge({option.courseId, current, "prerequisite", group.groupType, group.groupIndex});

                    if (distances.find(option.courseId) == distances.end()) {
                        distances[option.courseId] = currentDepth + 1;
                        pending.push({option.courseId, currentDepth + 1});
                    }
                }
            }
        }

        if (direction == "dependents" || direction == "both") {
            for (const DependentRelationship& dependent : dependentsFor(current)) {
                if (!graphNodeAllowed(dependent.courseId, false, subjects, colleges)) {
                    continue;
                }

                addNode(dependent.courseId);
                addEdge({current, dependent.courseId, "prerequisite", dependent.groupType, dependent.groupIndex});

                if (distances.find(dependent.courseId) == distances.end()) {
                    distances[dependent.courseId] = currentDepth + 1;
                    pending.push({dependent.courseId, currentDepth + 1});
                }
            }
        }
    }

    return graph;
}

std::string CsvCatalog::normalizeCourseId(const std::string& id) {
    std::stringstream input(trim(id));
    std::string part;
    std::vector<std::string> parts;
    while (input >> part) {
        parts.push_back(part);
    }

    std::ostringstream normalized;
    for (std::size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) {
            normalized << " ";
        }
        normalized << toUpper(parts[i]);
    }
    return normalized.str();
}

std::string CsvCatalog::deriveSubject(const std::string& id) {
    const std::string normalized = normalizeCourseId(id);
    const std::size_t separator = normalized.find(' ');
    if (separator != std::string::npos) {
        return normalized.substr(0, separator);
    }

    std::size_t end = 0;
    while (end < normalized.size() && !std::isdigit(static_cast<unsigned char>(normalized[end]))) {
        ++end;
    }
    return normalized.substr(0, end);
}

std::string CsvCatalog::normalizeSubject(const std::string& subject) {
    return toUpper(trim(subject));
}

std::string CsvCatalog::normalizeCollege(const std::string& college) {
    return toLower(trim(college));
}

void CsvCatalog::rebuildDependents() {
    dependentsByPrerequisite_.clear();

    for (const std::string& courseId : courseOrder_) {
        const auto found = courses_.find(courseId);
        if (found == courses_.end()) {
            continue;
        }

        for (const PrerequisiteGroup& group : prerequisiteGroupsFor(courseId)) {
            for (const PrerequisiteOption& option : group.options) {
                dependentsByPrerequisite_[option.courseId].push_back({
                    courseId,
                    group.groupType,
                    group.groupIndex,
                    false,
                });
            }
        }
    }
}

bool CsvCatalog::courseMatchesFilters(const CourseRecord& course, const CourseSearchFilters& filters) const {
    if (!filters.query.empty()
        && !containsCaseInsensitive(course.id, filters.query)
        && !containsCaseInsensitive(course.name, filters.query)
        && !containsCaseInsensitive(course.subject, filters.query)
        && !containsCaseInsensitive(course.college, filters.query)
        && !containsCaseInsensitive(course.department, filters.query)) {
        return false;
    }

    if (!filters.subjects.empty() && filters.subjects.find(normalizeSubject(course.subject)) == filters.subjects.end()) {
        return false;
    }

    if (!filters.colleges.empty() && filters.colleges.find(normalizeCollege(course.college)) == filters.colleges.end()) {
        return false;
    }

    return true;
}

bool CsvCatalog::graphNodeAllowed(
    const std::string& courseId,
    bool isRoot,
    const std::unordered_set<std::string>& subjects,
    const std::unordered_set<std::string>& colleges
) const {
    if (isRoot) {
        return true;
    }

    const CourseRecord* course = findCourse(courseId);
    const std::string subject = course == nullptr ? deriveSubject(courseId) : normalizeSubject(course->subject);
    const std::string college = course == nullptr ? "" : normalizeCollege(course->college);

    if (!subjects.empty() && subjects.find(subject) == subjects.end()) {
        return false;
    }

    if (!colleges.empty() && colleges.find(college) == colleges.end()) {
        return false;
    }

    return true;
}

GraphNode CsvCatalog::nodeFor(const std::string& courseId) const {
    const CourseRecord* course = findCourse(courseId);
    if (course == nullptr) {
        return {courseId, courseId, "", true, "", "", deriveSubject(courseId)};
    }

    return {
        course->id,
        course->id,
        course->name,
        false,
        course->college,
        course->department,
        course->subject,
    };
}

} // namespace api
