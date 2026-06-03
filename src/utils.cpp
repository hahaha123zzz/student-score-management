#include "utils.h"
#include <fstream>
#include <sstream>
#include <ctime>
#include <algorithm>
#include <map>
#include <cstdlib>
#include <direct.h>
#include <io.h>

namespace utils {

    // ===== 目录操作 =====

    void ensureDataDir() {
        if (_access(DATA_DIR.c_str(), 0) != 0) {
            _mkdir(DATA_DIR.c_str());
        }
    }

    bool fileExists(const std::string& path) {
        return _access(path.c_str(), 0) == 0;
    }

    // ===== DJB2 密码哈希 =====

    std::string hashPassword(const std::string& str) {
        size_t hash = 5381;
        for (size_t i = 0; i < str.size(); i++) {
            char c = str[i];
            hash = ((hash << 5) + hash) + (unsigned char)c;
        }
        std::stringstream ss;
        ss << std::hex << hash;
        return ss.str();
    }

    // ===== Token管理 (内存存储) =====

    static std::map<std::string, std::string> tokenToUserId;
    static std::map<std::string, std::string> userRoles;

    std::string getUserIdByToken(const std::string& token) {
        if (tokenToUserId.count(token)) return tokenToUserId[token];
        return "-1";
    }

    void storeToken(const std::string& token, const std::string& userId) {
        tokenToUserId[token] = userId;
    }

    bool isTokenValid(const std::string& token) {
        return tokenToUserId.count(token) > 0 && tokenToUserId[token] != "-1";
    }

    std::string getUserRoleByToken(const std::string& token) {
        std::string uid = getUserIdByToken(token);
        if (uid != "-1" && userRoles.count(uid)) return userRoles[uid];
        return "";
    }

    void setUserRole(const std::string& userId, const std::string& role) {
        userRoles[userId] = role;
    }

    // ===== Token生成 (rand + srand) =====

    std::string generateToken() {
        static bool seeded = false;
        if (!seeded) {
            srand((unsigned int)time(NULL));
            seeded = true;
        }
        const char hex[] = "0123456789abcdef";
        std::string token;
        for (int i = 0; i < 16; i++) {
            token += hex[rand() % 16];
        }
        return token;
    }

    // ===== 时间 =====

    std::string getCurrentTime() {
        time_t t = time(NULL);
        struct tm timeinfo;
        localtime_s(&timeinfo, &t);
        char buf[32];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
        return std::string(buf);
    }

    // ===== 字符串工具 =====

    std::string toLower(const std::string& s) {
        std::string r = s;
        std::transform(r.begin(), r.end(), r.begin(), ::tolower);
        return r;
    }

    std::string trim(const std::string& s) {
        size_t start = 0;
        while (start < s.size() && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r' || s[start] == '\n')) start++;
        size_t end = s.size();
        while (end > start && (s[end-1] == ' ' || s[end-1] == '\t' || s[end-1] == '\r' || s[end-1] == '\n')) end--;
        return s.substr(start, end - start);
    }

    std::vector<std::string> split(const std::string& s, char delim) {
        std::vector<std::string> result;
        std::string current;
        for (size_t i = 0; i < s.size(); i++) {
            if (s[i] == delim) {
                result.push_back(current);
                current.clear();
            } else {
                current += s[i];
            }
        }
        result.push_back(current);
        return result;
    }

    std::string join(const std::vector<std::string>& parts, const std::string& delim) {
        std::string result;
        for (size_t i = 0; i < parts.size(); i++) {
            if (i > 0) result += delim;
            result += parts[i];
        }
        return result;
    }

    bool startsWith(const std::string& s, const std::string& prefix) {
        return s.size() >= prefix.size() && s.substr(0, prefix.size()) == prefix;
    }

    // ===== 数学工具 =====

    double round2(double val) {
        val = val * 100;
        long long rounded = (long long)(val + 0.5);
        return (double)rounded / 100.0;
    }

    std::string gradeToLetter(double percent) {
        if (percent >= 90) return "A";
        if (percent >= 80) return "B";
        if (percent >= 70) return "C";
        if (percent >= 60) return "D";
        return "F";
    }

    double gradeToGPA(double percent) {
        if (percent >= 90) return 4.0;
        if (percent >= 85) return 3.7;
        if (percent >= 82) return 3.3;
        if (percent >= 78) return 3.0;
        if (percent >= 75) return 2.7;
        if (percent >= 72) return 2.3;
        if (percent >= 68) return 2.0;
        if (percent >= 64) return 1.5;
        if (percent >= 60) return 1.0;
        return 0.0;
    }

    // ===== JSON响应构建 (手动拼字符串) =====

    std::string jsonEscape(const std::string& s) {
        std::string r;
        for (size_t i = 0; i < s.size(); i++) {
            if (s[i] == '"') r += "\\\"";
            else if (s[i] == '\\') r += "\\\\";
            else if (s[i] == '\r') r += "\\r";
            else if (s[i] == '\n') r += "\\n";
            else if (s[i] == '\t') r += "\\t";
            else r += s[i];
        }
        return r;
    }

    std::string jsonResponse(bool success, const std::string& message, const std::string& data) {
        std::string r = "{\"success\":";
        r += success ? "true" : "false";
        r += ",\"message\":\"" + jsonEscape(message) + "\"";
        r += ",\"data\":" + data;
        r += "}";
        return r;
    }

    std::string errorResponse(const std::string& message) {
        return jsonResponse(false, message, "{}");
    }

    // ===== URL参数解析 =====

    std::string getQueryParam(const std::string& queryString, const std::string& key) {
        std::string searchKey = key + "=";
        size_t pos = queryString.find(searchKey);
        if (pos == std::string::npos) return "";
        pos += searchKey.size();
        size_t end = queryString.find("&", pos);
        if (end == std::string::npos) end = queryString.size();
        return queryString.substr(pos, end - pos);
    }

    // ===== 日志 =====

    void logAction(const std::string& userId, const std::string& action, const std::string& detail) {
        ensureDataDir();
        std::ofstream f(DATA_DIR + "logs.txt", std::ios::app);
        f << userId << "|" << action << "|" << detail << "|" << getCurrentTime() << "\n";
    }

    std::string getLogs(int page, int size, const std::string& userFilter) {
        return "{\"total\":0,\"page\":" + std::to_string(page) + ",\"size\":" + std::to_string(size) + ",\"data\":[]}";
    }

}
