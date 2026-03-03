#include "RobotTask.hpp"
#include <sstream>

RobotTask::RobotTask()
    : startSpaceId(""), endSpaceId(""), taskName("") {
}

RobotTask::RobotTask(const std::string& start, const std::string& end, const std::string& name)
    : startSpaceId(start), endSpaceId(end), taskName(name) {
}

std::string RobotTask::toString() const {
    std::ostringstream oss;
    oss << "RobotTask[" << taskName << ": " << startSpaceId << " -> " << endSpaceId << "]";
    return oss.str();
}
