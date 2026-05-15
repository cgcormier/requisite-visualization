#include "Graph.h"
#include "Course.h"
#include <vector>
#include <unordered_map>

Graph::Graph(){
     adj = std::unordered_map<std::string, std::vector<std::string>>();
     
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

    return distance[end] == INT_MAX ? -1 : distance[end];
}
std::vector<std::string> Graph::grabPreReqs(std::string id){
    std::ifstream file("courses.csv");
    if(!file.is_open()){
        std::cerr << "Error opening file!" << std::endl;
        return {};
    }
    std::vector<std::string> prereqs; 
    std::string line = "";

    while(std::getline(file, line)){
        std::stringstream ss(line);
        std::string name, course_id, credits, college, prereq_str;
        std::getline(ss, name, ',');
        std::getline(ss, course_id, ',');
        std::getline(ss, credits, ',');
        std::getline(ss, college, ',');
        std::getline(ss, prereq_str);

        if(course_id == id){
            std::stringstream prereq_ss(prereq_str);
            std::string prereq;
            while(std::getline(prereq_ss, prereq, '|')){
                prereqs.push_back(prereq);
            }
            break;
        }
    }

    return prereqs;
}
