#include "Graph.h"
#include "Course.h"
#include <cctype>
#include <cstdlib>
#include <vector>
#include <unordered_map>

namespace {
    std::string trim(const std::string& value) {
        size_t start = 0;
        while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
            ++start;
        }

        size_t end = value.size();
        while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
            --end;
        }

        return value.substr(start, end - start);
    }

    std::vector<std::string> parseCsvRow(const std::string& line) {
        std::vector<std::string> fields;
        std::string field;
        bool in_quotes = false;

        for (size_t i = 0; i < line.size(); ++i) {
            char current = line[i];

            if (in_quotes) {
                if (current == '"') {
                    if (i + 1 < line.size() && line[i + 1] == '"') {
                        field.push_back('"');
                        ++i;
                    } else {
                        in_quotes = false;
                    }
                } else {
                    field.push_back(current);
                }
            } else if (current == '"') {
                in_quotes = true;
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

    std::vector<std::string> splitCourseList(const std::string& value) {
        std::vector<std::string> courses;
        std::stringstream ss(value);
        std::string course;

        while (std::getline(ss, course, ',')) {
            course = trim(course);
            if (!course.empty()) {
                courses.push_back(course);
            }
        }

        return courses;
    }

    std::ifstream openCoursesFile() {
        const char* override_path = std::getenv("COURSES_CSV_PATH");
        const std::vector<std::string> candidate_paths = {
            override_path == nullptr ? "" : override_path,
            "backend/data/courses.csv",
            "data/courses.csv",
            "courses.csv",
        };

        for (const std::string& path : candidate_paths) {
            if (path.empty()) {
                continue;
            }

            std::ifstream file(path);
            if (file.is_open()) {
                return file;
            }
        }

        return {};
    }

    PrereqGroups parsePrereqGroups(const std::string& prereq_str) {
        PrereqGroups groups;
        std::string prereqs = trim(prereq_str);

        if (prereqs.empty()) {
            return groups;
        }

        size_t separator = prereqs.find('|');
        std::string and_section = separator == std::string::npos
            ? prereqs
            : prereqs.substr(0, separator);

        groups.andPrereqs = splitCourseList(and_section);

        if (separator == std::string::npos) {
            return groups;
        }

        std::string or_section = prereqs.substr(separator + 1);
        std::stringstream or_stream(or_section);
        std::string semicolon_group;

        while (std::getline(or_stream, semicolon_group, ';')) {
            std::stringstream legacy_pipe_stream(semicolon_group);
            std::string pipe_group;

            while (std::getline(legacy_pipe_stream, pipe_group, '|')) {
                std::vector<std::string> alternatives = splitCourseList(pipe_group);
                if (!alternatives.empty()) {
                    groups.orGroups.push_back(alternatives);
                }
            }
        }

        return groups;
    }
}

std::vector<std::string> PrereqGroups::allPrereqs() const {
    std::vector<std::string> prereqs = andPrereqs;

    for (const std::vector<std::string>& group : orGroups) {
        prereqs.insert(prereqs.end(), group.begin(), group.end());
    }

    return prereqs;
}

Graph::Graph(){
     adj = std::unordered_map<std::string, std::vector<std::string>>();

}

void Graph::buildGraph(){
    std::ifstream file = openCoursesFile();
    if(!file.is_open()){
        std::cerr << "Error opening courses CSV file!" << std::endl;
        return;
    }

    adj.clear();

    std::string line = "";
    while(std::getline(file, line)){
        std::vector<std::string> fields = parseCsvRow(line);
        if (fields.empty()) {
            continue;
        }

        std::string course_id = trim(fields[0]);

        if (course_id == "id" || course_id.empty()) {
            continue;
        }

        adj[course_id];
        std::vector<std::string> prereqs;
        if (fields.size() >= 5) {
            prereqs = parsePrereqGroups(fields[4]).allPrereqs();
        }

        for (const std::string& prereq : prereqs) {
            adj[prereq].push_back(course_id);
        }
    }

}

int Graph::CountPaths(std::string start, std::string end) { //bfs to find the shortest path between two courses
    std::unordered_map<std::string, bool> visited;
    std::unordered_map<std::string, int> distance;
    std::queue<std::string> q;

    for (auto& pair : adj) {
        visited[pair.first] = false;
        distance[pair.first] = INT_MAX;
    }

    visited[start] = true;
    distance[start] = 0;
    q.push(start);

    while (!q.empty()) {
        std::string current = q.front();
        q.pop();

        for (const std::string& neighbor : adj[current]) {
            if (!visited[neighbor]) {
                visited[neighbor] = true;
                distance[neighbor] = distance[current] + 1;
                q.push(neighbor);
            }
        }
    }

    auto end_distance = distance.find(end);
    return end_distance == distance.end() || end_distance->second == INT_MAX ? -1 : end_distance->second;
}
PrereqGroups Graph::grabPreReqGroups(std::string id){
    std::ifstream file = openCoursesFile();
    if(!file.is_open()){
        std::cerr << "Error opening courses CSV file!" << std::endl;
        return {};
    }
    std::string line = "";

    while(std::getline(file, line)){
        std::vector<std::string> fields = parseCsvRow(line);
        if (fields.size() < 5) {
            continue;
        }

        std::string course_id = trim(fields[0]);

        if (course_id == "id") {
            continue;
        }

        if(course_id == id){
            return parsePrereqGroups(fields[4]);
        }
    }

    return {};
}

std::vector<std::string> Graph::grabPreReqs(std::string id){
    return grabPreReqGroups(id).allPrereqs();
}
