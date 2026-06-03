#pragma once

#include <string>
#include <vector>

namespace stats {

    std::string getRank(const std::string& queryString);
    std::string getClassCompare(const std::string& queryString);
    std::string getDistribution(const std::string& queryString);
    std::string getTrend(const std::string& studentId);
    std::string getWarnings();
    std::string getEnrollmentStats();

}
