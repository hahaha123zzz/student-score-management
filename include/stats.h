#pragma once
#include "../lib/json.hpp"
#include <string>

using json = nlohmann::json;

namespace stats {
    json getRank(const std::string& examId, const std::string& type, const std::string& subject);
    json getClassCompare(const std::string& examId);
    json getDistribution(const std::string& examId, const std::string& subject);
    json getTrend(const std::string& studentId);
    json getWarnings();
    json convertGrade(const json& scores, const json& weightConfig);
}
