#include "Course.h"

const std::string& Course::getName() const {
    return name;
}

const std::string& Course::getId() const {
    return id;
}

int Course::getCredits() const {
    return credits;
}

const std::string& Course::getCollege() const {
    return college;
}

const std::vector<std::vector<std::string>>& Course::getPrereqs() const {
    return prereqs;
}
