#pragma once

#include <string>
#include <vector>

// ==================== storage 命名空间 ====================
// 本模块负责所有数据的持久化存取。系统没有使用 MySQL 等数据库，
// 而是采用 CSV（逗号分隔值，Comma-Separated Values）纯文本文件来存储数据。
// CSV 格式：每行是一条记录，行内用逗号分隔不同字段，类似 Excel 表格。
// 优点：简单、跨平台、可直接用 Excel 打开查看。

namespace storage {

    // ===== 通用 CSV 工具函数 =====

    // 按逗号拆分一行文本为多个字段（字符串数组）
    // 例如把 "101,张三,123456" 拆成 {"101","张三","123456"}
    std::vector<std::string> splitCSV(const std::string& line);

    // 转义字段中的特殊字符，使 CSV 写入格式正确
    // 如果字段本身包含逗号，用双引号包裹以防解析混乱
    // 例如 "张,三" → "\"张,三\""
    std::string escapeField(const std::string& field);

    // ===== 底层文件读写 =====
    // 这三个函数是本模块的基石，所有数据表的存取都建立在其上

    // 读取整个 CSV 文件，返回二维数组（每行是一个字符串数组）
    // 会自动跳过第一行（表头/列名），只返回数据行
    std::vector<std::vector<std::string>> readCSV(const std::string& filename);

    // 将二维数组写入 CSV 文件（覆盖写入，会清空原有内容）
    void writeCSV(const std::string& filename, const std::vector<std::vector<std::string>>& data);

    // 向 CSV 文件末尾追加一行（不覆盖原有内容）
    void appendCSVRow(const std::string& filename, const std::vector<std::string>& row);

    // ===== 用户表（users.csv）=====
    // 字段说明：id(用户编号) | name(用户名) | password(密码) | role(角色：admin/teacher/student) | phone(手机号) | created_at(创建时间)
    // 管理员、教师和学生共存在此表，通过 role 字段区分身份
    std::vector<std::vector<std::string>> getUsers();
    void saveUsers(const std::vector<std::vector<std::string>>& data);

    // ===== 学生表（students.csv）=====
    // 字段说明：id(学号) | name(姓名) | birthday(出生日期) | class(所在班级编号) | gender(性别) | enrolled_at(入学时间)
    // 此表记录学生基本信息，与班级表通过 class 字段关联
    std::vector<std::vector<std::string>> getStudents();
    void saveStudents(const std::vector<std::vector<std::string>>& data);

    // ===== 班级表（classes.csv）=====
    // 字段说明：id(班级编号) | name(班级名称) | grade(年级) | created_at(创建时间)
    // 例如：id=301, name="计算机科学1班", grade="2024级"
    std::vector<std::vector<std::string>> getClasses();
    void saveClasses(const std::vector<std::vector<std::string>>& data);

    // ===== 科目表（subjects.csv）=====
    // 字段说明：id(科目编号) | name(科目名称) | full_score(满分分值)
    // 例如：id=1, name="高等数学", full_score="100"
    std::vector<std::vector<std::string>> getSubjects();
    void saveSubjects(const std::vector<std::vector<std::string>>& data);

    // ===== 考试表（exams.csv）=====
    // 字段说明：id(考试编号) | name(考试名称) | date(考试日期) | status(状态：active/archived) | subjects(包含的科目列表) | weight_config(成绩权重配置)
    // 例如：id=1, name="2024年秋期中考试", date="2024-11-15", status="active"
    std::vector<std::vector<std::string>> getExams();
    void saveExams(const std::vector<std::vector<std::string>>& data);

    // ===== 成绩表（grades.csv）=====
    // 字段说明：id(成绩记录编号) | student_id(学号，关联学生表) | exam_id(考试编号，关联考试表) | scores(各科分数) | is_makeup(是否补考成绩) | submitted_by(录入人) | submitted_at(录入时间)
    // 成绩表是核心数据表，每行记录某学生在某次考试中的各科分数
    std::vector<std::vector<std::string>> getGrades();
    void saveGrades(const std::vector<std::vector<std::string>>& data);

}
