#pragma once

#include "PrerequisiteParser.h"
#include "api/ApiModels.h"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace api {

class CsvCatalog {
public:
    bool load(std::string* errorMessage = nullptr);
    bool loadFromFile(const std::string& path, std::string* errorMessage = nullptr);

    const std::string& sourcePath() const;
    std::size_t size() const;

    const CourseRecord* findCourse(const std::string& id) const;
    bool containsCourse(const std::string& id) const;

    std::vector<CourseRecord> searchCourses(const CourseSearchFilters& filters) const;
    std::vector<PrerequisiteGroup> prerequisiteGroupsFor(const std::string& id) const;
    std::vector<std::string> flattenedPrerequisitesFor(const std::string& id) const;
    std::vector<DependentRelationship> dependentsFor(const std::string& id) const;

    GraphResult graphFor(
        const std::string& rootCourseId,
        const std::string& direction,
        int depth,
        const std::unordered_set<std::string>& subjects,
        const std::unordered_set<std::string>& colleges
    ) const;

    static std::string normalizeCourseId(const std::string& id);
    static std::string deriveSubject(const std::string& id);
    static std::string normalizeSubject(const std::string& subject);
    static std::string normalizeCollege(const std::string& college);

private:
    struct StoredCourse {
        CourseRecord course;
        PrereqGroups prerequisites;
    };

    std::string sourcePath_;
    std::vector<std::string> courseOrder_;
    std::unordered_map<std::string, StoredCourse> courses_;
    std::unordered_map<std::string, std::vector<DependentRelationship>> dependentsByPrerequisite_;

    void rebuildDependents();
    bool courseMatchesFilters(const CourseRecord& course, const CourseSearchFilters& filters) const;
    bool graphNodeAllowed(
        const std::string& courseId,
        bool isRoot,
        const std::unordered_set<std::string>& subjects,
        const std::unordered_set<std::string>& colleges
    ) const;
    GraphNode nodeFor(const std::string& courseId) const;
};

} // namespace api
