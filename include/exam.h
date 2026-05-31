#pragma once
#include "../lib/json.hpp"

using json = nlohmann::json;

namespace exam {
    json getExams(const std::string& status = "");
    json addExam(const json& data);
    json updateExam(const std::string& id, const json& data);
    json deleteExam(const std::string& id);
    json publishExam(const std::string& id);
    json retractExam(const std::string& id);
}
