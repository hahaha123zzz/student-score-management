/*
 * EduGrade 基础部分（第 1 部分）
 *
 * 这一部分只放最底层的公共内容：
 * 1. 标准库和 httplib 头文件
 * 2. 基础数据结构
 * 3. 字符串、文件、时间等通用工具
 * 4. 表单解析与 JSON 字符串拼接
 *
 * 读代码时建议先看这一部分，再往后看数据读写和业务逻辑。
 *
 * 额外说明：
 * 为了减少编辑器把分段 .cpp 当成“独立源文件”时产生的大量误报，
 * 每个分段文件都加上了简单的包含保护，让它既能单独解析，也能被 main.cpp 组合。
 */

#ifndef EDUGRADE_CORE_PART1_BASE_CPP_INCLUDED
#define EDUGRADE_CORE_PART1_BASE_CPP_INCLUDED

#include "../lib/httplib.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

using httplib::Request;
using httplib::Response;
using httplib::Server;

namespace {

const std::string DATA_DIR = "data/";
const std::string USERS_FILE = DATA_DIR + "users.txt";
const std::string STUDENTS_FILE = DATA_DIR + "students.txt";
const std::string SUBJECTS_FILE = DATA_DIR + "subjects.txt";
const std::string EXAMS_FILE = DATA_DIR + "exams.txt";
const std::string GRADES_FILE = DATA_DIR + "grades.txt";
const std::string LOGS_FILE = DATA_DIR + "logs.txt";

/*
 * 按课程设计要求，这里保留 Student 类。
 * 这样既满足“学生类封装属性”的要求，也便于后面继续扩展成员函数。
 */
class Student {
public:
    std::string id;
    std::string name;
    std::string gender;
    std::string birthday;
    std::string class_name;
    std::string phone;
    std::string created_at;

    std::string gradeName() const {
        if (class_name.find("高一") != std::string::npos) return "高一";
        if (class_name.find("高二") != std::string::npos) return "高二";
        if (class_name.find("高三") != std::string::npos) return "高三";
        return "未知年级";
    }
};

struct User {
    std::string id;
    std::string name;
    std::string role;
    std::string password;
    std::string created_at;
};

struct Subject {
    std::string id;
    std::string name;
    int full_score;
};

struct Exam {
    std::string id;
    std::string name;
    std::string date;
    std::vector<std::string> subjects;
};

struct GradeRecord {
    std::string exam_id;
    std::string student_id;
    std::map<std::string, std::string> scores;
    std::string updated_at;
};

struct LogEntry {
    std::string id;
    std::string user_id;
    std::string action;
    std::string detail;
    std::string timestamp;
};

struct SessionInfo {
    std::string user_id;
    std::string name;
    std::string role;
};

struct RankItem {
    Student student;
    GradeRecord grade;
    double total;
    double average;
    int class_rank;
    int grade_rank;
};

std::map<std::string, SessionInfo> g_sessions;

/* ---------- 字符串与基础工具 ---------- */

std::string trim(const std::string& text) {
    std::size_t left = 0;
    std::size_t right = text.size();
    while (left < right && std::isspace(static_cast<unsigned char>(text[left]))) {
        ++left;
    }
    while (right > left && std::isspace(static_cast<unsigned char>(text[right - 1]))) {
        --right;
    }
    return text.substr(left, right - left);
}

std::vector<std::string> split(const std::string& text, char delimiter) {
    std::vector<std::string> parts;
    std::string current;
    std::size_t i = 0;
    for (i = 0; i < text.size(); ++i) {
        if (text[i] == delimiter) {
            parts.push_back(current);
            current.clear();
        } else {
            current.push_back(text[i]);
        }
    }
    parts.push_back(current);
    return parts;
}

std::string join(const std::vector<std::string>& items, const std::string& delimiter) {
    std::ostringstream out;
    for (std::size_t i = 0; i < items.size(); ++i) {
        if (i > 0) out << delimiter;
        out << items[i];
    }
    return out.str();
}

std::string toLowerAscii(const std::string& text) {
    std::string result = text;
    for (std::size_t i = 0; i < result.size(); ++i) {
        result[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(result[i])));
    }
    return result;
}

std::string formatDouble(double value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(2) << value;
    std::string text = out.str();
    while (!text.empty() && text.find('.') != std::string::npos && text[text.size() - 1] == '0') {
        text.erase(text.size() - 1);
    }
    if (!text.empty() && text[text.size() - 1] == '.') {
        text.erase(text.size() - 1);
    }
    if (text.empty()) return "0";
    return text;
}

double parseDouble(const std::string& text) {
    if (text.empty()) return 0.0;
    char* end_ptr = NULL;
    double value = std::strtod(text.c_str(), &end_ptr);
    if (end_ptr == text.c_str()) return 0.0;
    return value;
}

bool pathExists(const std::string& path) {
#ifdef _WIN32
    return _access(path.c_str(), 0) == 0;
#else
    return access(path.c_str(), F_OK) == 0;
#endif
}

void ensureDataDirectory() {
    if (pathExists(DATA_DIR)) return;
#ifdef _WIN32
    _mkdir(DATA_DIR.c_str());
#else
    mkdir(DATA_DIR.c_str(), 0755);
#endif
}

std::vector<std::string> readLines(const std::string& path) {
    std::vector<std::string> lines;
    std::ifstream input(path.c_str());
    if (!input.is_open()) return lines;

    std::string line;
    while (std::getline(input, line)) {
        line = trim(line);
        if (!line.empty()) {
            lines.push_back(line);
        }
    }
    return lines;
}

void writeLines(const std::string& path, const std::vector<std::string>& lines) {
    std::ofstream output(path.c_str(), std::ios::trunc);
    for (std::size_t i = 0; i < lines.size(); ++i) {
        output << lines[i] << "\n";
    }
}

std::string currentTimeText() {
    std::time_t now = std::time(NULL);
    std::tm time_info;
#ifdef _WIN32
    localtime_s(&time_info, &now);
#else
    localtime_r(&now, &time_info);
#endif
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &time_info);
    return buffer;
}

std::string randomToken() {
    static const char charset[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::string token = "tk_";
    for (int i = 0; i < 24; ++i) {
        token.push_back(charset[std::rand() % (sizeof(charset) - 1)]);
    }
    token += "_" + formatDouble(static_cast<double>(std::time(NULL)));
    return token;
}

/* ---------- 表单与参数解析 ---------- */

std::string urlDecode(const std::string& text) {
    std::string result;
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '+') {
            result.push_back(' ');
        } else if (text[i] == '%' && i + 2 < text.size()) {
            std::string hex = text.substr(i + 1, 2);
            char ch = static_cast<char>(std::strtol(hex.c_str(), NULL, 16));
            result.push_back(ch);
            i += 2;
        } else {
            result.push_back(text[i]);
        }
    }
    return result;
}

std::map<std::string, std::string> parseFormBody(const std::string& body) {
    std::map<std::string, std::string> values;
    std::vector<std::string> pairs = split(body, '&');
    for (std::size_t i = 0; i < pairs.size(); ++i) {
        std::size_t pos = pairs[i].find('=');
        std::string key = pos == std::string::npos ? pairs[i] : pairs[i].substr(0, pos);
        std::string value = pos == std::string::npos ? "" : pairs[i].substr(pos + 1);
        values[urlDecode(key)] = urlDecode(value);
    }
    return values;
}

std::string queryValue(const Request& req, const std::string& key) {
    return req.has_param(key) ? req.get_param_value(key) : "";
}

std::string formValue(const std::map<std::string, std::string>& form, const std::string& key) {
    std::map<std::string, std::string>::const_iterator it = form.find(key);
    if (it == form.end()) return "";
    return it->second;
}

/* ---------- JSON 字符串拼接工具 ---------- */

std::string escapeJson(const std::string& text) {
    std::ostringstream out;
    for (std::size_t i = 0; i < text.size(); ++i) {
        char ch = text[i];
        if (ch == '\\') out << "\\\\";
        else if (ch == '"') out << "\\\"";
        else if (ch == '\n') out << "\\n";
        else if (ch == '\r') out << "\\r";
        else if (ch == '\t') out << "\\t";
        else out << ch;
    }
    return out.str();
}

std::string jsonString(const std::string& text) {
    return "\"" + escapeJson(text) + "\"";
}

std::string jsonBool(bool value) {
    return value ? "true" : "false";
}

std::string fieldRaw(const std::string& key, const std::string& json_value) {
    return jsonString(key) + ":" + json_value;
}

std::string fieldString(const std::string& key, const std::string& value) {
    return fieldRaw(key, jsonString(value));
}

std::string fieldNumber(const std::string& key, double value) {
    return fieldRaw(key, formatDouble(value));
}

std::string fieldInteger(const std::string& key, int value) {
    return fieldRaw(key, formatDouble(static_cast<double>(value)));
}

std::string fieldBool(const std::string& key, bool value) {
    return fieldRaw(key, jsonBool(value));
}

std::string makeObject(const std::vector<std::string>& fields) {
    return "{" + join(fields, ",") + "}";
}

std::string makeArray(const std::vector<std::string>& items) {
    return "[" + join(items, ",") + "]";
}

std::string makeResponse(bool success, const std::string& message, const std::string& data_json) {
    std::vector<std::string> fields;
    fields.push_back(fieldBool("success", success));
    fields.push_back(fieldString("message", message));
    fields.push_back(fieldRaw("data", data_json));
    return makeObject(fields);
}

void addCors(Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
}

void setJson(Response& res, const std::string& json_text) {
    addCors(res);
    res.set_content(json_text, "application/json; charset=UTF-8");
}

void setError(Response& res, int status, const std::string& message) {
    res.status = status;
    setJson(res, makeResponse(false, message, "null"));
}

}  // 匿名命名空间结束

#endif

