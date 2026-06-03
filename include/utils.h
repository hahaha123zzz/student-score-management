#pragma once

#include <string>
#include <vector>

namespace utils {

    // 数据目录
    const std::string DATA_DIR = "data/";

    // 目录操作
    void ensureDataDir();
    bool fileExists(const std::string& path);

    // 密码哈希 (DJB2算法)
    std::string hashPassword(const std::string& str);

    // Token管理
    std::string generateToken();
    int getUserIdByToken(const std::string& token);
    void storeToken(const std::string& token, int userId);
    bool isTokenValid(const std::string& token);
    std::string getUserRoleByToken(const std::string& token);
    void setUserRole(int userId, const std::string& role);

    // 日志
    void logAction(const std::string& userId, const std::string& action, const std::string& detail);
    std::string getLogs(int page, int size, const std::string& userFilter);

    // 时间
    std::string getCurrentTime();

    // 字符串工具
    std::string toLower(const std::string& s);
    std::string trim(const std::string& s);
    std::vector<std::string> split(const std::string& s, char delim);
    std::string join(const std::vector<std::string>& parts, const std::string& delim);
    bool startsWith(const std::string& s, const std::string& prefix);

    // 数学工具
    double round2(double val);
    std::string gradeToLetter(double percent);
    double gradeToGPA(double percent);

    // JSON响应构建 (手动拼接字符串)
    std::string jsonResponse(bool success, const std::string& message, const std::string& data);
    std::string errorResponse(const std::string& message);
    std::string jsonEscape(const std::string& s);

    // URL参数解析
    std::string getQueryParam(const std::string& queryString, const std::string& key);

}
