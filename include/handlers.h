#pragma once

#include <string>

namespace handlers {

    // ===== 认证 =====
    std::string login(const std::string& body);
    std::string changePassword(const std::string& token, const std::string& body);
    std::string checkAuth(const std::string& token);

    // ===== 管理员权限 =====
    bool isAdmin(const std::string& token);

    // ===== 用户管理 =====
    std::string getUsers();
    std::string addUser(const std::string& body);
    std::string updateUser(const std::string& userId, const std::string& body);
    std::string deleteUser(const std::string& userId);

    // ===== 学生管理 =====
    std::string getStudents(const std::string& queryString);
    std::string addStudent(const std::string& body);
    std::string updateStudent(const std::string& id, const std::string& body);
    std::string deleteStudent(const std::string& id);

    // ===== 班级管理 =====
    std::string getClasses();
    std::string addClass(const std::string& body);
    std::string updateClass(const std::string& id, const std::string& body);
    std::string deleteClass(const std::string& id);

    // ===== 科目管理 =====
    std::string getSubjects();
    std::string addSubject(const std::string& body);

    // ===== 考试管理 =====
    std::string getExams(const std::string& queryString);
    std::string addExam(const std::string& body);
    std::string updateExam(const std::string& id, const std::string& body);
    std::string deleteExam(const std::string& id);
    std::string publishExam(const std::string& id);
    std::string retractExam(const std::string& id);

    // ===== 成绩管理 =====
    std::string setGrades(const std::string& body);
    std::string importCSV(const std::string& body);
    std::string exportCSV(const std::string& queryString);
    std::string lockGrades(const std::string& examId);
    std::string getGrades(const std::string& queryString);
    std::string getStudentGrades(const std::string& studentId);
    std::string markMakeup(const std::string& body);

    // ===== 日志 =====
    std::string getLogs(const std::string& queryString);

    // ===== 内部工具 =====
    std::string parseBodyField(const std::string& body, const std::string& key);

}
