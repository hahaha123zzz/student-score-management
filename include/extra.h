#pragma once
#include "../lib/json.hpp"
#include <string>

using json = nlohmann::json;

namespace extra {
    json setError(const json& data);
    json getErrors(const std::string& studentId, const std::string& examId, const std::string& subject, int page, int size);
    json getErrorStats(const std::string& studentId);
    json getImbalance(const std::string& studentId);
    json getAbilityMap(const std::string& studentId, const std::string& examId);
    json getReport(const std::string& studentId);
    json getSuggest(const std::string& studentId);
}
