#include "storage.h"
#include "utils.h"
#include <fstream>
#include <sstream>

namespace storage {

    std::vector<std::string> splitCSV(const std::string& line) {
        return utils::split(line, ',');
    }

    std::string escapeField(const std::string& field) {
        if (field.find(',') != std::string::npos)
            return "\"" + field + "\"";
        return field;
    }

    std::vector<std::vector<std::string>> readCSV(const std::string& filename) {
        std::vector<std::vector<std::string>> data;
        std::string path = utils::DATA_DIR + filename;
        std::ifstream f(path);
        if (!f.is_open()) return data;

        std::string line;
        // 跳过第一行 (列头)
        if (std::getline(f, line)) {
            // 已跳过header
        }
        while (std::getline(f, line)) {
            if (line.empty()) continue;
            data.push_back(splitCSV(line));
        }
        return data;
    }

    void writeCSV(const std::string& filename, const std::vector<std::vector<std::string>>& data) {
        utils::ensureDataDir();
        std::ofstream f(utils::DATA_DIR + filename);
        if (!f.is_open()) return;
        for (size_t i = 0; i < data.size(); i++) {
            for (size_t j = 0; j < data[i].size(); j++) {
                if (j > 0) f << ",";
                f << escapeField(data[i][j]);
            }
            f << "\n";
        }
    }

    void appendCSVRow(const std::string& filename, const std::vector<std::string>& row) {
        utils::ensureDataDir();
        std::ofstream f(utils::DATA_DIR + filename, std::ios::app);
        if (!f.is_open()) return;
        for (size_t j = 0; j < row.size(); j++) {
            if (j > 0) f << ",";
            f << escapeField(row[j]);
        }
        f << "\n";
    }

    // ===== 用户 =====

    std::vector<std::vector<std::string>> getUsers() {
        return readCSV("users.csv");
    }

    void saveUsers(const std::vector<std::vector<std::string>>& data) {
        std::vector<std::vector<std::string>> withHeader;
        withHeader.push_back({"id","name","password","role","phone","created_at"});
        for (size_t i = 0; i < data.size(); i++) withHeader.push_back(data[i]);
        writeCSV("users.csv", withHeader);
    }

    // ===== 学生 =====

    std::vector<std::vector<std::string>> getStudents() {
        return readCSV("students.csv");
    }

    void saveStudents(const std::vector<std::vector<std::string>>& data) {
        std::vector<std::vector<std::string>> withHeader;
        withHeader.push_back({"id","name","birthday","class","gender","enrolled_at"});
        for (size_t i = 0; i < data.size(); i++) withHeader.push_back(data[i]);
        writeCSV("students.csv", withHeader);
    }

    // ===== 班级 =====

    std::vector<std::vector<std::string>> getClasses() {
        return readCSV("classes.csv");
    }

    void saveClasses(const std::vector<std::vector<std::string>>& data) {
        std::vector<std::vector<std::string>> withHeader;
        withHeader.push_back({"id","name","grade","created_at"});
        for (size_t i = 0; i < data.size(); i++) withHeader.push_back(data[i]);
        writeCSV("classes.csv", withHeader);
    }

    // ===== 科目 =====

    std::vector<std::vector<std::string>> getSubjects() {
        return readCSV("subjects.csv");
    }

    void saveSubjects(const std::vector<std::vector<std::string>>& data) {
        std::vector<std::vector<std::string>> withHeader;
        withHeader.push_back({"id","name","full_score"});
        for (size_t i = 0; i < data.size(); i++) withHeader.push_back(data[i]);
        writeCSV("subjects.csv", withHeader);
    }

    // ===== 考试 =====

    std::vector<std::vector<std::string>> getExams() {
        return readCSV("exams.csv");
    }

    void saveExams(const std::vector<std::vector<std::string>>& data) {
        std::vector<std::vector<std::string>> withHeader;
        withHeader.push_back({"id","name","date","status","subjects","weight_config"});
        for (size_t i = 0; i < data.size(); i++) withHeader.push_back(data[i]);
        writeCSV("exams.csv", withHeader);
    }

    // ===== 成绩 =====

    std::vector<std::vector<std::string>> getGrades() {
        return readCSV("grades.csv");
    }

    void saveGrades(const std::vector<std::vector<std::string>>& data) {
        std::vector<std::vector<std::string>> withHeader;
        withHeader.push_back({"id","student_id","exam_id","scores","is_makeup","submitted_by","submitted_at"});
        for (size_t i = 0; i < data.size(); i++) withHeader.push_back(data[i]);
        writeCSV("grades.csv", withHeader);
    }

}
