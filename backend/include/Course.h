
#pragma once

#include <string>
#include <vector>

class Course{
    private: 
        std::string name;
        std::string id;
        int credits;
        std::string college;
        std::vector<std::vector<std::string>> prereqs;

    public: 
        Course(std::string name, std::string id, int credits, std::string college, std::vector<std::vector<std::string>> prereqs) 
        : name(name), id(id), credits(credits), college(college), prereqs(prereqs) {}

        const std::string& getName() const;
        const std::string& getId() const;
        int getCredits() const;
        const std::string& getCollege() const;
        const std::vector<std::vector<std::string>>& getPrereqs() const;
};
