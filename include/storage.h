#pragma once

#include <string>
#include <vector>

namespace storage {

    // 通用CSV工具
    std::vector<std::string> splitCSV(const std::string& line);
    std::string escapeField(const std::string& field);

    // 底层文件读写
    std::vector<std::vector<std::string>> readCSV(const std::string& filename);
    void writeCSV(const std::string& filename, const std::vector<std::vector<std::string>>& data);
    void appendCSVRow(const std::string& filename, const std::vector<std::string>& row);

    // 用户: id, name, password, role, phone, created_at
    std::vector<std::vector<std::string>> getUsers();
    void saveUsers(const std::vector<std::vector<std::string>>& data);

    // 学生: id, name, birthday, class, gender, enrolled_at
    std::vector<std::vector<std::string>> getStudents();
    void saveStudents(const std::vector<std::vector<std::string>>& data);

    // 班级: id, name, grade, created_at
    std::vector<std::vector<std::string>> getClasses();
    void saveClasses(const std::vector<std::vector<std::string>>& data);

    // 科目: id, name, full_score
    std::vector<std::vector<std::string>> getSubjects();
    void saveSubjects(const std::vector<std::vector<std::string>>& data);

    // 考试: id, name, date, status, subjects, weight_config
    std::vector<std::vector<std::string>> getExams();
    void saveExams(const std::vector<std::vector<std::string>>& data);

    // 成绩: id, student_id, exam_id, scores, is_makeup, submitted_by, submitted_at
    std::vector<std::vector<std::string>> getGrades();
    void saveGrades(const std::vector<std::vector<std::string>>& data);

}
