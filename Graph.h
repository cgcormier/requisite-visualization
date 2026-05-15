#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <queue>

class Graph{
    private:
        std::unordered_map<std::string, std::vector<std::string>> adj;

    public: 
        Graph(std::unordered_map<std::string, std::vector<std::string>> adj) : adj(adj) {}
        
        int CountPaths(std::string start, std::string end);

        std::vector<std::string>  grabPreReqs(std::string id);

};