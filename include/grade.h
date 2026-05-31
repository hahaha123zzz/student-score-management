#pragma once
#include "../lib/json.hpp"
#include <string>

using json = nlohmann::json;

namespace grade {
    json setGrades(const json& data);
    json importCSV(const std::string& csvContent, const std::string& examId);
    json exportCSV(const std::string& examId);
    json lockGrades(const std::string& examId);
    json getGrades(const std::string& examId, const std::string& studentId = "");
    json getStudentGrades(const std::string& studentId);
    json markMakeup(const std::string& studentId, const std::string& examId, const json& scores);
}
