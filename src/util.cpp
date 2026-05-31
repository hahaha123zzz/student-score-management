#include "util.h"
#include <fstream>
#include <sstream>
#include <random>
#include <ctime>
#include <iomanip>
#include <algorithm>
#ifdef _WIN32
#include <io.h>
#include <direct.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#endif

namespace util {

static bool dirExists(const std::string& path) {
#ifdef _WIN32
    return _access(path.c_str(), 0) == 0;
#else
    return access(path.c_str(), F_OK) == 0;
#endif
}

static void makeDir(const std::string& path) {
#ifdef _WIN32
    _mkdir(path.c_str());
#else
    mkdir(path.c_str(), 0755);
#endif
}

json readJSON(const std::string& filename) {
    std::string path = DATA_DIR + filename;
    std::ifstream f(path);
    if (!f.is_open()) return json::array();
    json data;
    try { f >> data; }
    catch (...) { return json::array(); }
    return data;
}

void writeJSON(const std::string& filename, const json& data) {
    std::string path = DATA_DIR + filename;
    if (!dirExists(DATA_DIR)) makeDir(DATA_DIR);
    std::ofstream f(path);
    f << data.dump(2);
}

std::string hasher(const std::string& str) {
    size_t hash = 5381;
    for (char c : str) hash = ((hash << 5) + hash) + static_cast<size_t>(static_cast<unsigned char>(c));
    std::stringstream ss;
    ss << std::hex << hash;
    return ss.str();
}

std::string genToken() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    const char* hex = "0123456789abcdef";
    std::string token;
    for (int i = 0; i < 32; i++)
        token += hex[dis(gen)];
    return token;
}

std::string getCurrentTime() {
    auto t = std::time(nullptr);
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

void logAction(const std::string& userId, const std::string& action, const std::string& detail) {
    json logs = readJSON("logs.json");
    json entry;
    entry["timestamp"] = getCurrentTime();
    entry["user_id"] = userId;
    entry["action"] = action;
    entry["detail"] = detail;
    logs.push_back(entry);
    writeJSON("logs.json", logs);
}

json getLogs(int page, int size, const std::string& userFilter) {
    json logs = readJSON("logs.json");
    json filtered = json::array();
    for (auto& l : logs) {
        if (!userFilter.empty()) {
            std::string uid = l.value("user_id", "");
            if (uid.find(userFilter) == std::string::npos) continue;
        }
        filtered.push_back(l);
    }
    std::reverse(filtered.begin(), filtered.end());
    int total = filtered.size();
    int start = (page - 1) * size;
    int end = std::min(start + size, (int)total);
    json result;
    result["total"] = total;
    result["page"] = page;
    result["size"] = size;
    result["data"] = json::array();
    for (int i = start; i < end; i++)
        result["data"].push_back(filtered[i]);
    return result;
}

std::string toLower(const std::string& s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(), ::tolower);
    return r;
}

double round2(double val) {
    return std::round(val * 100.0) / 100.0;
}

json gradeToRank(const std::string& score, double full) {
    double s = 0;
    try { s = std::stod(score); } catch (...) {}
    if (full <= 0) full = 100;
    double pct = s / full * 100;
    std::string rank;
    if (pct >= 90) rank = "A";
    else if (pct >= 80) rank = "B";
    else if (pct >= 70) rank = "C";
    else if (pct >= 60) rank = "D";
    else rank = "F";
    double gpa = 0;
    if (pct >= 90) gpa = 4.0;
    else if (pct >= 85) gpa = 3.7;
    else if (pct >= 82) gpa = 3.3;
    else if (pct >= 78) gpa = 3.0;
    else if (pct >= 75) gpa = 2.7;
    else if (pct >= 72) gpa = 2.3;
    else if (pct >= 68) gpa = 2.0;
    else if (pct >= 64) gpa = 1.5;
    else if (pct >= 60) gpa = 1.0;
    json result;
    result["percent"] = round2(pct);
    result["grade"] = rank;
    result["gpa"] = gpa;
    return result;
}

json jsonResponse(bool success, const std::string& msg, const json& data) {
    json resp;
    resp["success"] = success;
    resp["message"] = msg;
    resp["data"] = data;
    return resp;
}

json errorResponse(const std::string& msg) {
    return jsonResponse(false, msg);
}

std::string extractToken(const std::string& authHeader) {
    if (authHeader.length() > 7 && authHeader.substr(0, 7) == "Bearer ")
        return authHeader.substr(7);
    return "";
}

}
