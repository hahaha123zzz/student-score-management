#include "models.h"
#include "utils.h"
#include <cstdlib>

// ===================================================================
// Person 实现 —— 用户基类
// Person 是所有用户的公共基类，Admin 和 Student 都继承自它。
// ===================================================================

// 默认构造函数：所有字符串成员初始化为空串，角色默认为 "student"
Person::Person() : m_id(""), m_name(""), m_password(""), m_role("student"), m_createdAt("") {}

// 带参构造函数：使用初始化列表为所有成员赋值
// 初始化列表（: 后面的部分）比在函数体里赋值更高效，因为减少了默认构造+赋值的开销
Person::Person(const std::string& id, const std::string& name, const std::string& password,
               const std::string& role, const std::string& createdAt)
    : m_id(id), m_name(name), m_password(password), m_role(role), m_createdAt(createdAt) {}

// ---- getter 方法：直接返回对应成员变量的值 ----
// 返回值类型用 const 引用（const std::string&）可以避免不必要的拷贝
// 末尾的 const 表示这个方法不会修改对象的任何成员
std::string Person::getId() const { return m_id; }
std::string Person::getName() const { return m_name; }
std::string Person::getPassword() const { return m_password; }
std::string Person::getRole() const { return m_role; }
std::string Person::getCreatedAt() const { return m_createdAt; }

// ---- setter 方法：修改对应成员变量的值 ----
// 通过 setter 修改数据的好处：可以在修改前做校验（如检查有效性），
// 而直接暴露 public 变量则无法控制修改行为
void Person::setName(const std::string& name) { m_name = name; }
void Person::setPassword(const std::string& password) { m_password = password; }
void Person::setRole(const std::string& role) { m_role = role; }

// 密码验证：对用户输入的明文密码做哈希计算，再与存储的哈希值比较
// 这样做的好处是：即使数据文件泄露，也无法直接看到用户的真实密码（只存储哈希）
bool Person::verifyPassword(const std::string& input) const {
    return utils::hashPassword(input) == m_password;
}

// 序列化：将 Person 对象转换为 CSV 格式的字符串数组
// Person 写入 6 个字段，其中第 5 个字段为空（占位，用于和学生数据格式对齐）
// 实际写入顺序：id, name, password, role, "", createdAt
std::vector<std::string> Person::toCSV() const {
    std::vector<std::string> row;
    row.push_back(m_id);
    row.push_back(m_name);
    row.push_back(m_password);
    row.push_back(m_role);
    row.push_back("");          // 占位字段，学生类会在此位置存生日
    row.push_back(m_createdAt);
    return row;
}

// 反序列化：将 CSV 字符串数组还原为 Person 对象
// static 方法可以不依赖已有对象直接调用
// 第 0 个 = id, 第 1 个 = name, 第 2 个 = password, 第 3 个 = role
// 第 5 个 = createdAt（如果存在），第 4 个字段（生日占位）被跳过
Person Person::fromCSV(const std::vector<std::string>& fields) {
    if (fields.size() < 4) return Person();    // 数据不完整则返回空对象
    return Person(fields[0], fields[1], fields[2], fields[3],
                  fields.size() > 5 ? fields[5] : "");
}

// ===================================================================
// Admin 实现 —— 管理员类
// Admin 继承 Person，只是把角色固定为 "admin"。
// ===================================================================

// 默认构造：调用 Person 的默认构造，再覆盖角色为 "admin"
Admin::Admin() : Person() { m_role = "admin"; }
// 带参构造：直接调用 Person 的带参构造，角色固定传 "admin"
Admin::Admin(const std::string& id, const std::string& name, const std::string& password,
             const std::string& createdAt)
    : Person(id, name, password, "admin", createdAt) {}

// ===================================================================
// Student 实现 —— 学生类
// Student 继承 Person，并新增班级、生日、性别、入学时间四个属性。
// ===================================================================

// 默认构造：先初始化父类 Person，再初始化自己新增的成员为空串
Student::Student() : Person(), m_className(""), m_birthday(""), m_gender(""), m_enrolledAt("") {}

// 从 CSV 字段数组构造
// fields 顺序：fields[0]=id, fields[1]=name, fields[2]=birthday,
//              fields[3]=className, fields[4]=gender, fields[5]=enrolledAt
// 角色固定为 "student"，密码初始为空（学生初始无密码）
Student::Student(const std::vector<std::string>& fields) {
    if (fields.size() < 6) return;    // 数据不完整则不构造
    m_id = fields[0];
    m_name = fields[1];
    m_birthday = fields[2];
    m_className = fields[3];
    m_gender = fields[4];
    m_enrolledAt = fields[5];
    m_role = "student";    // 学生角色固定
    m_password = "";       // 密码留空，由管理员后续设置
}

// ---- getter 方法 ----
std::string Student::getClassName() const { return m_className; }
std::string Student::getBirthday() const { return m_birthday; }
std::string Student::getGender() const { return m_gender; }

// ---- setter 方法 ----
void Student::setClassName(const std::string& cls) { m_className = cls; }
void Student::setBirthday(const std::string& birthday) { m_birthday = birthday; }
void Student::setGender(const std::string& gender) { m_gender = gender; }

// 序列化：将 Student 转换为 CSV 行
// 6 个字段：id, name, birthday, className, gender, enrolledAt
// 注意与 Person 的序列化不同：Student 多出了 birthday 等字段
std::vector<std::string> Student::toCSV() const {
    std::vector<std::string> row;
    row.push_back(m_id);
    row.push_back(m_name);
    row.push_back(m_birthday);
    row.push_back(m_className);
    row.push_back(m_gender);
    row.push_back(m_enrolledAt);
    return row;
}

// 反序列化：委托给从 fields 构造的构造函数
Student Student::fromCSV(const std::vector<std::string>& fields) {
    return Student(fields);
}

// ===================================================================
// Cls 实现 —— 班级类
// ===================================================================

// 默认构造：所有成员初始化为空串
Cls::Cls() : m_id(""), m_name(""), m_grade(""), m_createdAt("") {}

// 带参构造
Cls::Cls(const std::string& id, const std::string& name,
         const std::string& grade, const std::string& createdAt)
    : m_id(id), m_name(name), m_grade(grade), m_createdAt(createdAt) {}

std::string Cls::getId() const { return m_id; }
std::string Cls::getName() const { return m_name; }
std::string Cls::getGrade() const { return m_grade; }

void Cls::setName(const std::string& name) { m_name = name; }
void Cls::setGrade(const std::string& grade) { m_grade = grade; }

// 序列化：4 个字段 —— id, name, grade, createdAt
std::vector<std::string> Cls::toCSV() const {
    std::vector<std::string> row;
    row.push_back(m_id);
    row.push_back(m_name);
    row.push_back(m_grade);
    row.push_back(m_createdAt);
    return row;
}

// 反序列化：从 4 个字段还原 Cls 对象
Cls Cls::fromCSV(const std::vector<std::string>& fields) {
    if (fields.size() < 4) return Cls();    // 数据不足则返回空对象
    return Cls(fields[0], fields[1], fields[2], fields[3]);
}

// ===================================================================
// Subject 实现 —— 科目类
// ===================================================================

// 默认构造：满分默认为 100（大多数考试科目的满分）
Subject::Subject() : m_id(""), m_name(""), m_fullScore(100) {}

// 带参构造
Subject::Subject(const std::string& id, const std::string& name, int fullScore)
    : m_id(id), m_name(name), m_fullScore(fullScore) {}

std::string Subject::getId() const { return m_id; }
std::string Subject::getName() const { return m_name; }
int Subject::getFullScore() const { return m_fullScore; }

// 序列化：3 个字段 —— id, name, fullScore
// fullScore 是 int 类型，需要用 std::to_string 转为字符串
std::vector<std::string> Subject::toCSV() const {
    std::vector<std::string> row;
    row.push_back(m_id);
    row.push_back(m_name);
    row.push_back(std::to_string(m_fullScore));    // int → string 转换
    return row;
}

// 反序列化：从 3 个字段还原，第 3 个字段用 std::stoi 转回 int
Subject Subject::fromCSV(const std::vector<std::string>& fields) {
    if (fields.size() < 3) return Subject();
    return Subject(fields[0], fields[1], std::stoi(fields[2]));   // string → int 转换
}

// ===================================================================
// Exam 实现 —— 考试类
// ===================================================================

// 默认构造：状态默认为 "draft"（草稿状态，尚未发布）
Exam::Exam() : m_id(""), m_name(""), m_date(""), m_status("draft"),
               m_subjects(""), m_weightConfig("") {}

// 带参构造
Exam::Exam(const std::string& id, const std::string& name, const std::string& date,
           const std::string& status, const std::string& subjects, const std::string& weightConfig)
    : m_id(id), m_name(name), m_date(date), m_status(status),
      m_subjects(subjects), m_weightConfig(weightConfig) {}

std::string Exam::getId() const { return m_id; }
std::string Exam::getName() const { return m_name; }
std::string Exam::getDate() const { return m_date; }
std::string Exam::getStatus() const { return m_status; }
std::string Exam::getSubjects() const { return m_subjects; }
std::string Exam::getWeightConfig() const { return m_weightConfig; }

// 获取科目列表：将 "语文|数学|英语" 这样的字符串按 '|' 拆分成数组
std::vector<std::string> Exam::getSubjectList() const {
    return utils::split(m_subjects, '|');
}

void Exam::setName(const std::string& name) { m_name = name; }
void Exam::setDate(const std::string& date) { m_date = date; }
void Exam::setStatus(const std::string& status) { m_status = status; }
void Exam::setSubjects(const std::string& subjects) { m_subjects = subjects; }
void Exam::setWeightConfig(const std::string& wc) { m_weightConfig = wc; }

// 发布考试：将状态从 draft 改为 published
// 已锁定（locked）的考试不能再发布，返回 false 表示失败
bool Exam::publish() {
    if (m_status == "locked") return false;
    m_status = "published";
    return true;
}

// 撤回考试：将状态从 published 退回到 draft
// 已锁定（locked）的考试不能再撤回
bool Exam::retract() {
    if (m_status == "locked") return false;
    m_status = "draft";
    return true;
}

// 锁定考试：将状态设为 locked
// 锁定后考试不可再修改，是一种保护已完成考试的机制
bool Exam::lock() {
    m_status = "locked";
    return true;
}

// 根据科目名查找该科目的权重系数
// weightConfig 格式："科目名:满分:权重|科目名:满分:权重|..."
// 例如："数学:150:1.2|语文:150:1.0"  表示数学满分150权重1.2，语文满分150权重1.0
// 如果找不到对应科目，默认返回 1.0
double Exam::getSubjectWeight(const std::string& subject) const {
    std::vector<std::string> configs = utils::split(m_weightConfig, '|');
    for (size_t i = 0; i < configs.size(); i++) {
        std::vector<std::string> parts = utils::split(configs[i], ':');
        if (parts.size() >= 3 && parts[0] == subject)
            return std::stod(parts[2]);    // parts[2] 是权重值
    }
    return 1.0;    // 找不到则返回默认权重 1.0
}

// 根据科目名查找该科目的满分值
// 格式同上，parts[1] 是满分值
// 如果找不到对应科目，默认返回 100.0
double Exam::getSubjectFull(const std::string& subject) const {
    std::vector<std::string> configs = utils::split(m_weightConfig, '|');
    for (size_t i = 0; i < configs.size(); i++) {
        std::vector<std::string> parts = utils::split(configs[i], ':');
        if (parts.size() >= 3 && parts[0] == subject)
            return std::stod(parts[1]);    // parts[1] 是满分值
    }
    return 100.0;    // 找不到则返回默认满分 100.0
}

// 序列化：6 个字段 —— id, name, date, status, subjects, weightConfig
std::vector<std::string> Exam::toCSV() const {
    std::vector<std::string> row;
    row.push_back(m_id);
    row.push_back(m_name);
    row.push_back(m_date);
    row.push_back(m_status);
    row.push_back(m_subjects);
    row.push_back(m_weightConfig);
    return row;
}

// 反序列化：从 6 个字段还原 Exam 对象
Exam Exam::fromCSV(const std::vector<std::string>& fields) {
    if (fields.size() < 6) return Exam();
    return Exam(fields[0], fields[1], fields[2], fields[3], fields[4], fields[5]);
}

// ===================================================================
// Grade 实现 —— 成绩类
// ===================================================================

// 默认构造：所有成员初始化为默认值
Grade::Grade() : m_id(""), m_studentId(""), m_examId(""), m_scores(""), m_isMakeup(false),
                  m_submittedBy(""), m_submittedAt("") {}

// 从 CSV 字段数组构造
// fields 顺序：id, studentId, examId, scores, isMakeup, submittedBy, submittedAt
// isMakeup 支持多种格式："true" 或 "1" 都视为补考
Grade::Grade(const std::vector<std::string>& fields) {
    if (fields.size() < 7) return;    // 数据不完整则不构造
    m_id = fields[0];
    m_studentId = fields[1];
    m_examId = fields[2];
    m_scores = fields[3];
    m_isMakeup = (fields[4] == "true" || fields[4] == "1");   // 兼容两种布尔表示
    m_submittedBy = fields[5];
    m_submittedAt = fields[6];
}

std::string Grade::getRowId() const { return m_id; }
std::string Grade::getStudentId() const { return m_studentId; }
std::string Grade::getExamId() const { return m_examId; }
std::string Grade::getScores() const { return m_scores; }
bool Grade::getIsMakeup() const { return m_isMakeup; }

// 获取第 index 科的成绩（index 从 0 开始计数）
// 先调用 getScoreList() 解析所有成绩，再按下标取第 index 个
// 如果 index 越界则返回 0
double Grade::getScoreByIndex(int index) const {
    std::vector<double> list = getScoreList();
    if (index >= 0 && index < (int)list.size()) return list[index];
    return 0;
}

// 解析成绩字符串：将 "85|92|78" 拆分成 {85.0, 92.0, 78.0}
// 如果某段无法转为数字（如解析异常），则用 0 替代，保证不会因脏数据崩溃
std::vector<double> Grade::getScoreList() const {
    std::vector<double> result;
    std::vector<std::string> parts = utils::split(m_scores, '|');    // 按 | 拆分
    for (size_t i = 0; i < parts.size(); i++) {
        try {
            result.push_back(std::stod(parts[i]));    // string → double
        } catch (...) {
            result.push_back(0);   // 解析失败时填 0，保证程序稳定性
        }
    }
    return result;
}

void Grade::setScores(const std::string& scores) { m_scores = scores; }
void Grade::setIsMakeup(bool makeup) { m_isMakeup = makeup; }

// 计算加权总分
// 算法：遍历每科成绩，用公式 加权总分 = Σ(得分/满分 × 权重) / Σ权重 × 100 计算
// 这样可以公平比较不同满分值和不同权重的科目
// 例如：数学得分90（满分150，权重1.2），语文得分130（满分150，权重1.0）
//       加权分 = (90/150×1.2 + 130/150×1.0) / (1.2+1.0) × 100 ≈ 71.96
double Grade::calcTotal(const std::string& weightConfig) const {
    std::vector<double> scores = getScoreList();                     // 各科实际得分
    std::vector<std::string> configs = utils::split(weightConfig, '|');   // 权重配置
    double totalWeighted = 0, totalWeight = 0;
    for (size_t i = 0; i < scores.size() && i < configs.size(); i++) {
        std::vector<std::string> parts = utils::split(configs[i], ':');
        double full = (parts.size() >= 2 ? std::stod(parts[1]) : 100.0);    // 满分值
        double weight = (parts.size() >= 3 ? std::stod(parts[2]) : 1.0);    // 权重
        totalWeighted += (scores[i] / full) * weight;    // 累加：得分比例 × 权重
        totalWeight += weight;                           // 累加总权重
    }
    return totalWeight > 0 ? (totalWeighted / totalWeight) * 100 : 0;   // 换算回百分制
}

// 计算简单平均分：所有科目分数之和 ÷ 科目数
// 这是最直观的平均分算法，不考虑各科满分值和权重的差异
double Grade::calcAverage() const {
    std::vector<double> list = getScoreList();
    if (list.empty()) return 0;    // 没有成绩则返回 0
    double sum = 0;
    for (size_t i = 0; i < list.size(); i++) sum += list[i];
    return sum / list.size();      // 算术平均
}

// 序列化：将 Grade 转为 7 个 CSV 字段
// isMakeup 是 bool，序列化时转成字符串 "true" 或 "false"
std::vector<std::string> Grade::toCSV() const {
    std::vector<std::string> row;
    row.push_back(m_id);
    row.push_back(m_studentId);
    row.push_back(m_examId);
    row.push_back(m_scores);
    row.push_back(m_isMakeup ? "true" : "false");   // bool → string
    row.push_back(m_submittedBy);
    row.push_back(m_submittedAt);
    return row;
}

// 反序列化：委托给从 fields 构造的构造函数
Grade Grade::fromCSV(const std::vector<std::string>& fields) {
    return Grade(fields);
}
