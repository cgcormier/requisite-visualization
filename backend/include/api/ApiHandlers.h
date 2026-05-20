#pragma once

#include "api/ApiModels.h"
#include "api/CsvCatalog.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace api {

class ApiHandlers {
public:
    using QueryParams = std::unordered_map<std::string, std::vector<std::string>>;

    explicit ApiHandlers(const CsvCatalog& catalog);

    ApiResponse handleRequest(const std::string& method, const std::string& target) const;

private:
    const CsvCatalog& catalog_;

    ApiResponse handleCourses(const QueryParams& query) const;
    ApiResponse handleCourseRoute(const std::string& path) const;
    ApiResponse handleGraph(const QueryParams& query) const;

    CourseSearchFilters parseCourseFilters(const QueryParams& query, std::string& errorCode, std::string& errorMessage) const;
};

} // namespace api
