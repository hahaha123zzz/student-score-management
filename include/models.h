#pragma once

#include <string>
#include <vector>

// ===================================================================
// Person —— 用户基类
// 代表系统中的"人"（用户），存储所有用户共有的基本信息。
// 这是 Admin 和 Student 的父类，定义了公共属性和行为。
// 使用 protected 而不是 private，是为了让子类能直接访问这些成员。
// ===================================================================
class Person {
protected:
    std::string m_id;        // 用户唯一标识（学号/工号）
    std::string m_name;      // 用户姓名
    std::string m_password;  // 登录密码（存储的是哈希后的密文，不是明文）
    std::string m_role;      // 角色："admin"（管理员）或 "student"（学生）
    std::string m_createdAt; // 账号创建时间

public:
    // 默认构造函数：创建一个空的 Person，默认角色为 "student"
    Person();
    // 带参构造函数：用指定值初始化所有成员变量
    Person(const std::string& id, const std::string& name, const std::string& password,
           const std::string& role, const std::string& createdAt);

    // ---- getter 方法：读取私有/保护成员的只读接口 ----
    std::string getId() const;
    std::string getName() const;
    std::string getPassword() const;
    std::string getRole() const;
    std::string getCreatedAt() const;

    // ---- setter 方法：修改私有/保护成员的写入接口 ----
    void setName(const std::string& name);
    void setPassword(const std::string& password);
    void setRole(const std::string& role);

    // 验证密码是否正确：将用户输入的明文密码做哈希后，与存储的密文比较
    bool verifyPassword(const std::string& input) const;

    // 将当前对象序列化为 CSV 行（一个字符串数组）
    // 序列化：把对象在内存中的数据转换成可以写入文件的格式
    std::vector<std::string> toCSV() const;
    // 从 CSV 行反序列化为 Person 对象（静态方法，不需要已有对象就能调用）
    // 反序列化：把从文件读出的字符串还原成内存中的对象
    static Person fromCSV(const std::vector<std::string>& fields);
};

// ===================================================================
// Admin —— 管理员类，继承自 Person
// 管理员是系统中的超级用户，可以管理学生、班级、考试和成绩。
// 它不增加任何新属性（所有属性都继承自 Person），
// 只是将角色固定为 "admin" 以示与普通学生的区别。
// ===================================================================
class Admin : public Person {
public:
    // 默认构造函数：创建一个管理员，角色自动设为 "admin"
    Admin();
    // 带参构造函数：用指定值初始化，角色自动设为 "admin"
    Admin(const std::string& id, const std::string& name, const std::string& password,
          const std::string& createdAt);
};

// ===================================================================
// Student —— 学生类，继承自 Person
// 学生是系统中的普通用户，可以查看自己的成绩等信息。
// 在 Person 的基础上新增了班级、生日、性别、入学时间等学生特有属性。
// ===================================================================
class Student : public Person {
private:
    std::string m_className;  // 所属班级名称
    std::string m_birthday;   // 出生日期
    std::string m_gender;     // 性别（"男" / "女"）
    std::string m_enrolledAt; // 入学时间

public:
    // 默认构造函数
    Student();
    // 从 CSV 字段数组构造（常用于从文件读取学生数据）
    Student(const std::vector<std::string>& fields);

    // ---- getter 方法 ----
    std::string getClassName() const;
    std::string getBirthday() const;
    std::string getGender() const;

    // ---- setter 方法 ----
    void setClassName(const std::string& cls);
    void setBirthday(const std::string& birthday);
    void setGender(const std::string& gender);

    // 序列化：将学生对象转为 CSV 行（6 个字段：id, name, birthday, class, gender, enrolledAt）
    std::vector<std::string> toCSV() const;
    // 反序列化：从 CSV 行还原学生对象
    static Student fromCSV(const std::vector<std::string>& fields);
};

// ===================================================================
// Cls —— 班级类
// 代表学校中的一个班级，包含班级的基本信息。
// 注意：类名用 Cls 而不是 Class，因为 "class" 是 C++ 的关键字。
// ===================================================================
class Cls {
private:
    std::string m_id;        // 班级唯一标识
    std::string m_name;      // 班级名称（如 "计算机2101"）
    std::string m_grade;     // 年级（如 "2021"）
    std::string m_createdAt; // 班级创建时间

public:
    // 默认构造函数
    Cls();
    // 带参构造函数
    Cls(const std::string& id, const std::string& name,
        const std::string& grade, const std::string& createdAt);

    // ---- getter 方法 ----
    std::string getId() const;
    std::string getName() const;
    std::string getGrade() const;

    // ---- setter 方法 ----
    void setName(const std::string& name);
    void setGrade(const std::string& grade);

    // 序列化为 CSV 行（4 个字段：id, name, grade, createdAt）
    std::vector<std::string> toCSV() const;
    // 从 CSV 行反序列化
    static Cls fromCSV(const std::vector<std::string>& fields);
};

// ===================================================================
// Subject —— 科目类
// 代表一门考试科目（如"高等数学"、"大学英语"），包含科目名称和满分值。
// ===================================================================
class Subject {
private:
    std::string m_id;    // 科目唯一标识
    std::string m_name;  // 科目名称（如 "高等数学"）
    int m_fullScore;     // 满分值（如 100 或 150）

public:
    // 默认构造函数，满分默认 100
    Subject();
    // 带参构造函数
    Subject(const std::string& id, const std::string& name, int fullScore);

    // ---- getter 方法 ----
    std::string getId() const;
    std::string getName() const;
    int getFullScore() const;

    // 序列化为 CSV 行（3 个字段：id, name, fullScore）
    std::vector<std::string> toCSV() const;
    // 从 CSV 行反序列化
    static Subject fromCSV(const std::vector<std::string>& fields);
};

// ===================================================================
// Exam —— 考试类
// 代表一次考试安排（如"2024年期中考试"），包含考试名称、日期、状态、
// 包含哪些科目、以及各科目的满分值和权重配置。
// 状态有三种：draft（草稿）、published（已发布）、locked（已锁定）
// ===================================================================
class Exam {
private:
    std::string m_id;           // 考试唯一标识
    std::string m_name;         // 考试名称（如 "期中考试"）
    std::string m_date;         // 考试日期
    std::string m_status;       // 状态："draft" / "published" / "locked"
    std::string m_subjects;     // 包含的科目列表，用 '|' 分隔（如 "数学|英语|物理"）
    std::string m_weightConfig; // 权重配置，格式："科目:满分:权重|..."

public:
    // 默认构造函数，状态默认 "draft"
    Exam();
    // 带参构造函数
    Exam(const std::string& id, const std::string& name, const std::string& date,
         const std::string& status, const std::string& subjects, const std::string& weightConfig);

    // ---- getter 方法 ----
    std::string getId() const;
    std::string getName() const;
    std::string getDate() const;
    std::string getStatus() const;
    std::string getSubjects() const;
    std::string getWeightConfig() const;

    // 将 subjects 字符串按 '|' 拆分成科目名称列表
    std::vector<std::string> getSubjectList() const;

    // ---- setter 方法 ----
    void setName(const std::string& name);
    void setDate(const std::string& date);
    void setStatus(const std::string& status);
    void setSubjects(const std::string& subjects);
    void setWeightConfig(const std::string& wc);

    // 发布考试（draft → published），已锁定的考试不能发布
    bool publish();
    // 撤回考试（published → draft），已锁定的考试不能撤回
    bool retract();
    // 锁定考试（任何状态 → locked），锁定后不可再修改
    bool lock();

    // 根据科目名查找该科目在本次考试中的权重
    double getSubjectWeight(const std::string& subject) const;
    // 根据科目名查找该科目在本次考试中的满分值
    double getSubjectFull(const std::string& subject) const;

    // 序列化为 CSV 行（6 个字段：id, name, date, status, subjects, weightConfig）
    std::vector<std::string> toCSV() const;
    // 从 CSV 行反序列化
    static Exam fromCSV(const std::vector<std::string>& fields);
};

// ===================================================================
// Grade —— 成绩类
// 代表一名学生在一次考试中的成绩记录。
// 包含各科分数（用 '|' 分隔）、是否为补考成绩、提交人、提交时间等。
// 提供计算加权总分和平均分的方法。
// ===================================================================
class Grade {
private:
    std::string m_id;          // 成绩记录唯一标识
    std::string m_studentId;   // 对应学生的 ID
    std::string m_examId;      // 对应考试的 ID
    std::string m_scores;      // 各科分数，用 '|' 分隔（如 "85|92|78"）
    bool m_isMakeup;           // 是否为补考成绩
    std::string m_submittedBy; // 提交人（录入成绩的管理员）
    std::string m_submittedAt; // 提交时间

public:
    // 默认构造函数
    Grade();
    // 从 CSV 字段数组构造（常用于从文件读取成绩数据）
    Grade(const std::vector<std::string>& fields);

    // ---- getter 方法 ----
    std::string getRowId() const;
    std::string getStudentId() const;
    std::string getExamId() const;
    std::string getScores() const;
    bool getIsMakeup() const;

    // 获取第 index 科的成绩（index 从 0 开始）
    double getScoreByIndex(int index) const;
    // 将 scores 字符串按 '|' 拆分，返回每科成绩的浮点数列表
    std::vector<double> getScoreList() const;

    // ---- setter 方法 ----
    void setScores(const std::string& scores);
    void setIsMakeup(bool makeup);

    // 计算加权总分：根据 weightConfig 中配置的满分和权重，计算加权百分制总分
    // 公式：加权总分 = Σ(得分/满分 × 权重) / Σ权重 × 100
    double calcTotal(const std::string& weightConfig) const;
    // 计算简单平均分（各科分数的算术平均）
    double calcAverage() const;

    // 序列化为 CSV 行（7 个字段：id, studentId, examId, scores, isMakeup, submittedBy, submittedAt）
    std::vector<std::string> toCSV() const;
    // 从 CSV 行反序列化
    static Grade fromCSV(const std::vector<std::string>& fields);
};
