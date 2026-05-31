#pragma once
#include "../lib/json.hpp"
#include <string>
#include <vector>

using json = nlohmann::json;

namespace util {
    const std::string DATA_DIR = "data/";

    json readJSON(const std::string& filename);
    void writeJSON(const std::string& filename, const json& data);
    std::string hasher(const std::string& str);
    std::string genToken();
    void logAction(const std::string& userId, const std::string& action, const std::string& detail);
    json getLogs(int page, int size, const std::string& userFilter);
    std::string getCurrentTime();
    std::string toLower(const std::string& s);
    double round2(double val);
    json gradeToRank(const std::string& score, double fullScore);
    json jsonResponse(bool success, const std::string& msg, const json& data = json::object());
    json errorResponse(const std::string& msg);
    std::string extractToken(const std::string& authHeader);
}
