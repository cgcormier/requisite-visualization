#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include <climits>

struct PrereqGroups {
    std::vector<std::string> andPrereqs;
    std::vector<std::vector<std::string>> orGroups;

    std::vector<std::string> allPrereqs() const;
};

class Graph{
    private:
        std::unordered_map<std::string, std::vector<std::string>> adj;

    public: 
        Graph();

        Graph(std::unordered_map<std::string, std::vector<std::string>> adj) : adj(adj) {}
        
        void buildGraph();
        int CountPaths(std::string start, std::string end);

        PrereqGroups grabPreReqGroups(std::string id);
        std::vector<std::string>  grabPreReqs(std::string id);

};
