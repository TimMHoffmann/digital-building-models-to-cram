#pragma once
#include <string>

class RobotTask {
public:
    std::string startSpaceId;
    std::string endSpaceId;
    std::string taskName;

    RobotTask();
    RobotTask(const std::string& start, const std::string& end, const std::string& name);
    
    std::string toString() const;
};
