#pragma once

#include <string>

// =====================================================================
// handlers 命名空间 —— 所有 HTTP API 的业务逻辑处理函数
// 每个函数接收 JSON 请求体（body 参数，一个 JSON 字符串）或查询参数
// 返回 JSON 格式的响应字符串，结构统一为：
//   {"success": true/false, "message": "提示信息", "data": {...}}
// =====================================================================

namespace handlers {

    // ===== 认证模块（登录/修改密码/验证身份）=====

    // 用户登录 —— 处理 POST /api/login
    // 输入：body = {"username":"xxx", "password":"xxx"}
    // 返回：{"success":true, "data":{"token":"令牌","role":"角色"}} 或错误信息
    std::string login(const std::string& body);

    // 修改密码 —— 处理 POST /api/changePassword
    // 输入：token（请求头中的登录凭证）, body = {"old_password":"旧密码", "new_password":"新密码"}
    // 要求新密码至少4位，需先验证旧密码正确
    std::string changePassword(const std::string& token, const std::string& body);

    // 验证登录状态 —— 处理 GET /api/checkAuth
    // 输入：token（登录令牌）
    // 返回：当前用户的 user_id 和 role，若 token 无效则返回"未登录"
    std::string checkAuth(const std::string& token);

    // ===== 管理员权限 =====

    // 判断当前 token 是否属于管理员
    // 返回 true 表示该用户有管理员权限（role == "admin"），false 表示无权限
    bool isAdmin(const std::string& token);

    // ===== 用户管理（管理员功能）=====

    // 获取所有用户列表 —— GET /api/users
    // 返回：用户数组 [{id, name, role, created_at}, ...]
    std::string getUsers();

    // 添加新用户 —— POST /api/users
    // 输入：body = {"id":"用户ID", "name":"姓名", "role":"admin/student"}
    // 默认角色为 student，默认密码为 123456（加密存储）
    std::string addUser(const std::string& body);

    // 更新用户信息 —— PUT /api/users/{userId}
    // 输入：userId（URL路径中的用户ID）, body = {"name":"新姓名", "role":"新角色"}
    // 只更新 body 中提供的字段，未提供的字段保持不变
    std::string updateUser(const std::string& userId, const std::string& body);

    // 删除用户 —— DELETE /api/users/{userId}
    // 输入：userId（要删除的用户ID）
    std::string deleteUser(const std::string& userId);

    // ===== 学生管理 =====

    // 获取学生列表（支持搜索、按班级筛选、分页）—— GET /api/students?search=关键词&class=班级名&page=1&size=20
    // 返回：{total: 总数, page: 当前页, size: 每页条数, data: [学生对象...]}
    std::string getStudents(const std::string& queryString);

    // 添加学生 —— POST /api/students
    // 输入：body = {"id":"学号", "name":"姓名", "class":"班级", "gender":"性别", "birthday":"生日"}
    std::string addStudent(const std::string& body);

    // 更新学生信息 —— PUT /api/students/{学号}
    // 输入：id（URL中的学号）, body = {"name":"新姓名", "class":"新班级", ...}
    std::string updateStudent(const std::string& id, const std::string& body);

    // 删除学生 —— DELETE /api/students/{学号}
    std::string deleteStudent(const std::string& id);

    // ===== 班级管理 =====

    // 获取所有班级列表 —— GET /api/classes
    std::string getClasses();

    // 添加班级 —— POST /api/classes
    // 输入：body = {"name":"班级名称", "grade":"年级"}
    std::string addClass(const std::string& body);

    // 更新班级 —— PUT /api/classes/{班级ID}
    std::string updateClass(const std::string& id, const std::string& body);

    // 删除班级 —— DELETE /api/classes/{班级ID}
    std::string deleteClass(const std::string& id);

    // ===== 科目管理 =====

    // 获取所有科目列表 —— GET /api/subjects
    // 返回：[{id, name, full_score}, ...]，full_score 为满分值（默认100）
    std::string getSubjects();

    // 添加科目 —— POST /api/subjects
    // 输入：body = {"name":"科目名称", "full_score":150}
    std::string addSubject(const std::string& body);

    // 更新科目 —— PUT /api/subjects/{科目ID}
    std::string updateSubject(const std::string& id, const std::string& body);

    // 删除科目 —— DELETE /api/subjects/{科目ID}
    std::string deleteSubject(const std::string& id);

    // ===== 考试管理 =====

    // 获取考试列表（可按状态筛选）—— GET /api/exams?status=draft/published/locked
    // 每次考试包含科目列表和各科目的满分/权重配置
    std::string getExams(const std::string& queryString);

    // 创建考试 —— POST /api/exams
    // 输入：body = {"name":"考试名称", "date":"2024-06-15"}
    // 默认包含语数英三科，各150分满分、权重1.0，状态为 draft（草稿）
    std::string addExam(const std::string& body);

    // 更新考试信息 —— PUT /api/exams/{考试ID}
    std::string updateExam(const std::string& id, const std::string& body);

    // 删除考试 —— DELETE /api/exams/{考试ID}
    std::string deleteExam(const std::string& id);

    // 发布考试成绩 —— PUT /api/exams/{考试ID}/publish
    // 将考试状态从 draft 改为 published，学生端可查看成绩
    // 注意：locked 状态的考试不能发布
    std::string publishExam(const std::string& id);

    // 撤回已发布的成绩 —— PUT /api/exams/{考试ID}/retract
    // 将考试状态从 published 改回 draft，学生端不可见
    // 注意：locked 状态的考试不能撤回
    std::string retractExam(const std::string& id);

    // ===== 成绩管理 =====

    // 录入/修改单个学生的成绩 —— POST /api/grades
    // 输入：body = {"exam_id":"考试ID", "student_id":"学号", "scores":"85,92,78"}
    // ★核心覆盖逻辑：如果该学生在此考试已有成绩，则直接覆盖旧成绩；
    // 如果没有记录，则新增一条。这就是"覆盖式录入"的实现方式。
    std::string setGrades(const std::string& body);

    // CSV批量导入成绩 —— POST /api/grades/import（当前为预留接口）
    // 输入：body 中包含 CSV 格式的成绩数据
    std::string importCSV(const std::string& body);

    // 导出成绩为CSV格式 —— GET /api/grades/export?exam_id=考试ID
    // 返回：{csv: "学号,姓名,语文,数学,英语\r\n..."} 格式的CSV字符串
    std::string exportCSV(const std::string& queryString);

    // 锁定成绩 —— POST /api/grades/lock/{考试ID}
    // 锁定后成绩不可再修改、发布或撤回，用于数据归档
    std::string lockGrades(const std::string& examId);

    // 查询成绩列表 —— GET /api/grades?exam_id=考试ID&student_id=学号（可选）
    std::string getGrades(const std::string& queryString);

    // 查询某个学生的所有成绩 —— GET /api/grades/{学号}
    std::string getStudentGrades(const std::string& studentId);

    // 录入补考成绩 —— POST /api/grades/makeup
    // 输入：body = {"student_id":"学号", "exam_id":"考试ID", "scores":"补考分数,..."}
    // 将 is_makeup 字段标记为 true，区别于正常考试成绩
    std::string markMakeup(const std::string& body);

    // ===== 操作日志 =====

    // 获取操作日志 —— GET /api/logs?page=1
    std::string getLogs(const std::string& queryString);

    // ===== 内部工具函数 =====

    // 从 JSON 字符串中提取指定字段的值
    // 例：body = {"name":"张三","age":18}, key="name" → 返回 "张三"
    // 支持字符串类型和数字类型的值提取
    std::string parseBodyField(const std::string& body, const std::string& key);

    // 根据学号查找学生信息（姓名、班级），返回逗号分隔: "姓名,班级"
    std::string findStudentInfo(const std::string& studentId);

    // 通用删除逻辑：从CSV表中按ID删除一行
    std::string genericDelete(const std::string& filename, const std::string& id);

}
