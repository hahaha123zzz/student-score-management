#include "utils.h"
#include "storage.h"
#include "models.h"
#include "handlers.h"
#include "stats.h"
#include "server.h"
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>

// ============================================================
// 【跨平台路径】获取可执行文件所在目录
// ============================================================
// 程序需要知道自己的位置，才能找到 web/ 前端文件目录。
// 不同操作系统获取程序路径的方法完全不同：
//   Windows：GetModuleFileNameA() 获取 exe 完整路径
//   macOS：  _NSGetExecutablePath() 获取程序路径
//   Linux：  读取 /proc/self/exe 符号链接
// ============================================================
#ifdef _WIN32
    #include <windows.h>       // GetModuleFileNameA / MAX_PATH
    #define PATH_SEPARATOR "\\"
#elif defined(__APPLE__)
    #include <mach-o/dyld.h>   // _NSGetExecutablePath
    #include <limits.h>        // PATH_MAX
    #include <stdlib.h>        // realpath
    #define PATH_SEPARATOR "/"
#else
    #include <unistd.h>        // readlink
    #include <limits.h>        // PATH_MAX
    #define PATH_SEPARATOR "/"
#endif

// ============================================================
// 【系统初始化】seedFullData —— 生成模拟测试数据
// ============================================================
// 当程序首次运行时，data/ 目录下没有任何 CSV 文件。
// 本函数负责创建以下数据文件并填充示例数据：
//
//   文件            内容说明
//   ────────────   ─────────────────────────────────
//   classes.csv     9 个班级（高一/高二/高三各3个班）
//   subjects.csv    9 门科目（语数英150分 + 理化生史政地100分）
//   students.csv    50 名学生（覆盖9个班级）
//   users.csv       学生用户 + 4 个管理员/老师账号（密码统一123456）
//   exams.csv       3 场考试（期中→月考→期末，科目逐渐增多）
//   grades.csv      50人×3场 = 150 条成绩记录（随机生成但可复现）
//
// 设计思路：
//   ① 先检查核心文件是否已存在，若已存在则跳过初始化（幂等性）
//   ② 用 srand(42) 固定随机种子，保证每次生成的成绩数据一致
//   ③ 学生编号规律：S2024xxx = 高一，S2025xxx = 高二，S2026xxx = 高三
//   ④ 学生水平分6级(1-6)，1为优等生、6为后进生，影响随机成绩的基准
//   ⑤ 成绩生成公式：基准百分比 + 学生偏移 + 科目偏移 + 考试难度偏移
//      - 基准百分比由 level 决定（优等生95%起，后进生45%起）
//      - 学生偏移：(si×7 + subi×13 + ei×17) % 21 - 10，约 -10 ~ +10
//      - 考试难度：ei × 3，第2场比第1场难3%，第3场比第1场难6%
//      - 最终限定在 [15%, 99%] 范围内
// ============================================================
static void seedFullData() {
    // 确保 data/ 目录存在
    utils::ensureDataDir();

    // ===== 班级 =====
    // 9 个班级，命名采用常见的"年级+数字+班"格式
    // 高一1班 ~ 高三3班，覆盖高中三个年级
    if (!utils::fileExists(utils::DATA_DIR + "classes.csv")) {
        std::string classNames[9] = {
            "高一1班","高一2班","高一3班",
            "高二1班","高二2班","高二3班",
            "高三1班","高三2班","高三3班"
        };
        std::string grades[9] = {"高一","高一","高一","高二","高二","高二","高三","高三","高三"};
        std::vector<std::vector<std::string>> data;
        // CSV 表头：id, name, grade, created_at
        data.push_back({"id","name","grade","created_at"});
        for (int i = 0; i < 9; i++) {
            std::vector<std::string> row;
            row.push_back("CLS" + std::to_string(i+1));   // 班级ID：CLS1~CLS9
            row.push_back(classNames[i]);
            row.push_back(grades[i]);                       // 所属年级
            row.push_back("2024-09-01");                    // 创建日期
            data.push_back(row);
        }
        storage::writeCSV("classes.csv", data);
    }

    // ===== 科目 =====
    // 9 门科目，语文/数学/英语满分150分，其余科目满分100分
    if (!utils::fileExists(utils::DATA_DIR + "subjects.csv")) {
        std::string subNames[9] = {"语文","数学","英语","物理","化学","生物","历史","政治","地理"};
        int fullScores[9] = {150,150,150,100,100,100,100,100,100};
        std::vector<std::vector<std::string>> data;
        // CSV 表头：id, name, full_score
        data.push_back({"id","name","full_score"});
        for (int i = 0; i < 9; i++) {
            std::vector<std::string> row;
            row.push_back("SUB" + std::to_string(i+1));    // 科目ID：SUB1~SUB9
            row.push_back(subNames[i]);
            row.push_back(std::to_string(fullScores[i]));
            data.push_back(row);
        }
        storage::writeCSV("subjects.csv", data);
    }

    // ===== 50个学生 =====
    // 二维数组，每行：[学号, 姓名, 出生日期, 班级, 性别, 入学日期, 水平等级]
    // 水平等级（第7个字段）：1=优等, 2=良好, 3=中等偏上, 4=中等, 5=中等偏下, 6=后进
    // 学号规律：S2024xxx=高一，S2025xxx=高二，S2026xxx=高三
    std::string raw[50][7] = {
        {"S2024001","张三","2007-03-15","高一1班","男","2024-09-01","1"},
        {"S2024002","李四","2007-04-20","高一1班","女","2024-09-01","2"},
        {"S2024003","王明","2007-05-10","高一1班","男","2024-09-01","3"},
        {"S2024004","陈丽","2007-06-05","高一1班","女","2024-09-01","4"},
        {"S2024005","刘洋","2007-07-12","高一1班","男","2024-09-01","5"},
        {"S2024006","赵敏","2007-08-01","高一1班","女","2024-09-01","1"},
        {"S2024007","孙伟","2007-03-20","高一2班","男","2024-09-01","2"},
        {"S2024008","周芳","2007-04-25","高一2班","女","2024-09-01","3"},
        {"S2024009","吴强","2007-05-30","高一2班","男","2024-09-01","1"},
        {"S2024010","郑婷","2007-06-15","高一2班","女","2024-09-01","2"},
        {"S2024011","冯军","2007-07-20","高一2班","男","2024-09-01","3"},
        {"S2024012","何静","2007-08-25","高一2班","女","2024-09-01","4"},
        {"S2024013","沈浩","2007-03-10","高一3班","男","2024-09-01","5"},
        {"S2024014","韩雪","2007-04-15","高一3班","女","2024-09-01","1"},
        {"S2024015","秦勇","2007-05-20","高一3班","男","2024-09-01","2"},
        {"S2024016","许莉","2007-06-25","高一3班","女","2024-09-01","3"},
        {"S2024017","邓超","2007-07-30","高一3班","男","2024-09-01","1"},
        {"S2024018","彭蕾","2007-08-05","高一3班","女","2024-09-01","2"},
        {"S2025001","马飞","2007-03-15","高二1班","男","2024-09-01","1"},
        {"S2025002","宋婷","2007-04-20","高二1班","女","2024-09-01","2"},
        {"S2025003","唐龙","2007-05-10","高二1班","男","2024-09-01","3"},
        {"S2025004","曹娜","2007-06-05","高二1班","女","2024-09-01","4"},
        {"S2025005","高翔","2007-07-12","高二1班","男","2024-09-01","5"},
        {"S2025006","罗琳","2007-08-01","高二2班","女","2024-09-01","1"},
        {"S2025007","梁志","2007-03-20","高二2班","男","2024-09-01","2"},
        {"S2025008","谢慧","2007-04-25","高二2班","女","2024-09-01","3"},
        {"S2025009","黄磊","2007-05-30","高二2班","男","2024-09-01","4"},
        {"S2025010","朱玥","2007-06-15","高二2班","女","2024-09-01","5"},
        {"S2025011","徐明","2007-07-20","高二3班","男","2024-09-01","1"},
        {"S2025012","胡燕","2007-08-25","高二3班","女","2024-09-01","2"},
        {"S2025013","林峰","2007-03-10","高二3班","男","2024-09-01","3"},
        {"S2025014","郭丽","2007-04-15","高二3班","女","2024-09-01","4"},
        {"S2025015","蔡刚","2007-05-20","高二3班","男","2024-09-01","5"},
        {"S2026001","潘玉","2007-06-25","高三1班","女","2024-09-01","1"},
        {"S2026002","蒋文","2007-07-30","高三1班","男","2024-09-01","2"},
        {"S2026003","余磊","2007-08-05","高三1班","男","2024-09-01","3"},
        {"S2026004","苏芳","2007-03-15","高三1班","女","2024-09-01","4"},
        {"S2026005","叶波","2007-04-20","高三1班","男","2024-09-01","5"},
        {"S2026006","姜涛","2007-05-10","高三2班","男","2024-09-01","1"},
        {"S2026007","董兰","2007-06-05","高三2班","女","2024-09-01","2"},
        {"S2026008","程浩","2007-07-12","高三2班","男","2024-09-01","3"},
        {"S2026009","魏敏","2007-08-01","高三2班","女","2024-09-01","4"},
        {"S2026010","袁帅","2007-03-20","高三3班","男","2024-09-01","1"},
        {"S2026011","谌洁","2007-04-25","高三3班","女","2024-09-01","2"},
        {"S2026012","欧阳","2007-05-30","高三3班","女","2024-09-01","3"},
        {"S2026013","段鹏","2007-06-15","高三3班","男","2024-09-01","4"},
        {"S2026014","靳悦","2007-07-20","高三3班","女","2024-09-01","5"},
        {"S2026015","长江","2007-08-25","高三3班","男","2024-09-01","1"},
        {"S2026016","黄河","2007-03-10","高三3班","女","2024-09-01","2"},
    };

    // 分别检查 students.csv 和 users.csv 是否需要创建
    bool needStudents = !utils::fileExists(utils::DATA_DIR + "students.csv");
    bool needUsers = !utils::fileExists(utils::DATA_DIR + "users.csv");

    std::vector<std::vector<std::string>> studentData;
    std::vector<std::vector<std::string>> userData;

    if (needStudents) {
        // students.csv 表头：id, name, birthday, class, gender, enrolled_at
        studentData.push_back({"id","name","birthday","class","gender","enrolled_at"});
    }
    if (needUsers) {
        // users.csv 表头：id, name, password, role, phone, created_at
        userData.push_back({"id","name","password","role","phone","created_at"});
        // 创建 4 个管理员/教师账号（密码统一为 123456 的哈希值）
        userData.push_back({"admin","系统管理员",utils::hashPassword("123456"),"admin","","2024-09-01"});
        userData.push_back({"T001","张明老师",utils::hashPassword("123456"),"admin","","2024-09-01"});
        userData.push_back({"T002","李华老师",utils::hashPassword("123456"),"admin","","2024-09-01"});
        userData.push_back({"T003","王芳老师",utils::hashPassword("123456"),"admin","","2024-09-01"});
    }

    // 遍历 34 名学生的原始数据，同时生成 students.csv 和 users.csv 的行
    for (int i = 0; i < 34; i++) {
        if (needStudents) {
            std::vector<std::string> row;
            for (int j = 0; j < 6; j++) row.push_back(raw[i][j]);   // 取前 6 个字段
            studentData.push_back(row);
        }
        if (needUsers) {
            // 每个学生对应一个登录账号：学号为用户名，密码 123456，角色 student
            userData.push_back({raw[i][0], raw[i][1], utils::hashPassword("123456"), "student", "", "2024-09-01"});
        }
    }

    if (needStudents) storage::writeCSV("students.csv", studentData);
    if (needUsers) storage::writeCSV("users.csv", userData);

    // ===== 考试 =====
    // 3 场考试，科目数逐渐增多（5→7→9 门），模拟真实学校的考试安排
    // weight_config 格式：科目名:满分:权重，用 | 分隔多个科目
    //   例如 "语文:150:1.0" 表示语文满分150，权重1.0（完整计入总分）
    //   物理权重0.8 表示实际计入总分时打八折
    if (!utils::fileExists(utils::DATA_DIR + "exams.csv")) {
        std::vector<std::vector<std::string>> data;
        // CSV 表头：id, name, date, status, subjects, weight_config
        data.push_back({"id","name","date","status","subjects","weight_config"});
        // 期中考试：5 门主科（语数英物化）
        data.push_back({"EXAM1","高一上学期期中考试","2025-11-10","published",
            "语文|数学|英语|物理|化学",
            "语文:150:1.0|数学:150:1.0|英语:150:1.0|物理:100:0.8|化学:100:0.8"});
        // 第二次月考：新增生物、历史
        data.push_back({"EXAM2","高一上学期第二次月考","2025-12-20","published",
            "语文|数学|英语|物理|化学|生物|历史",
            "语文:150:1.0|数学:150:1.0|英语:150:1.0|物理:100:0.8|化学:100:0.8|生物:100:0.6|历史:100:0.6"});
        // 期末考试：涵盖全部 9 门科目
        data.push_back({"EXAM3","高一上学期期末考试","2026-01-15","published",
            "语文|数学|英语|物理|化学|生物|历史|政治|地理",
            "语文:150:1.0|数学:150:1.0|英语:150:1.0|物理:100:0.8|化学:100:0.8|生物:100:0.6|历史:100:0.6|政治:100:0.6|地理:100:0.6"});
        storage::writeCSV("exams.csv", data);
    }

    // ===== 模拟成绩 =====
    // 为每个学生 × 每场考试生成一条成绩记录
    if (!utils::fileExists(utils::DATA_DIR + "grades.csv")) {
        srand(42);   // 固定随机种子，保证每次运行生成的成绩数据完全一致

        // 三场考试各自的科目列表（与 exams.csv 中的 subjects 字段对应）
        std::string examSubs[3] = {
            "语文|数学|英语|物理|化学",
            "语文|数学|英语|物理|化学|生物|历史",
            "语文|数学|英语|物理|化学|生物|历史|政治|地理"
        };

        std::vector<std::vector<std::string>> data;
        // CSV 表头：id, student_id, exam_id, scores, is_makeup, submitted_by, submitted_at
        data.push_back({"id","student_id","exam_id","scores","is_makeup","submitted_by","submitted_at"});
        int gradeId = 1;   // 自增 ID

        // 外循环：遍历 3 场考试（ei = 0, 1, 2）
        for (int ei = 0; ei < 3; ei++) {
            // 将科目字符串拆分为数组，如 ["语文","数学","英语","物理","化学"]
            std::vector<std::string> subList = utils::split(examSubs[ei], '|');

            // 内循环：遍历 50 名学生（si = 0, 1, ..., 49）
            for (int si = 0; si < 34; si++) {
                // 获取该学生的水平等级（1~6）
                int level = std::stoi(raw[si][6]);
                // 基准百分比：level 1→95%, level 2→85%, ..., level 6→45%
                int basePct[6] = {0, 95, 85, 75, 60, 45};
                std::string scores;
                // 为该考试每门科目生成一个分数
                for (int subi = 0; subi < (int)subList.size(); subi++) {
                    // 确定该科目的满分：语文/数学/英语 150，其余 100
                    int full = (subList[subi] == "语文" || subList[subi] == "数学" || subList[subi] == "英语") ? 150 : 100;
                    // 计算偏移量，使不同学生、不同科目、不同考试之间分数有合理差异
                    // (si * 7 + subi * 13 + ei * 17) % 21 保证结果在 0~20 之间
                    // 减 10 使其范围变为 -10 ~ +10
                    int variation = (si * 7 + subi * 13 + ei * 17) % 21 - 10;
                    // 最终百分比 = 基准 - 考试难度递减(3%/场) + 随机偏移
                    int pct = basePct[level] + variation - ei * 3;
                    // 限制在合理范围内
                    if (pct > 99) pct = 99;
                    if (pct < 15) pct = 15;
                    // 多个科目的分数用 | 连接，如 "120|130|110|85|90"
                    if (subi > 0) scores += "|";
                    scores += std::to_string((full * pct) / 100);
                }

                data.push_back({
                    std::to_string(gradeId++),
                    raw[si][0],                               // 学号
                    "EXAM" + std::to_string(ei + 1),          // 考试ID
                    scores,                                     // 分数列表
                    "false",                                    // 非补考
                    "system",                                   // 由系统自动录入
                    utils::getCurrentTime()                     // 提交时间
                });
            }
        }
        storage::writeCSV("grades.csv", data);
    }

    std::cout << "测试数据初始化完成（50名学生、3场考试、9个班级）" << std::endl;
}

// ============================================================
// extractId —— 从 RESTful 路径中提取资源 ID
// ============================================================
// 例如：path="/api/users/admin", prefix="/api/users" → 返回 "admin"
// 例如：path="/api/exams/EXAM1", prefix="/api/exams"  → 返回 "EXAM1"
// 要求 path 比 prefix 至少多 2 个字符（多出 "/?" 和一个实际字符）
// ============================================================
static std::string extractId(const std::string& path, const std::string& prefix) {
    if (path.size() > prefix.size() + 1 && path.substr(0, prefix.size()) == prefix)
        return path.substr(prefix.size() + 1);   // +1 跳过前缀后的 "/"
    return "";
}

// ============================================================
// 【路由分发】route —— HTTP 请求的中央调度器
// ============================================================
// 这是整个系统的"交通指挥中心"。每个 HTTP 请求到达后，
// 由 server.cpp 调用本函数，根据 路径(path) + 方法(method)
// 决定由哪个函数来处理这个请求。
//
// 路由设计采用 if-else 链判断模式（简化版的路由表），
// 按功能模块分为以下几组：
//
//   ┌──────────────┬────────────────────────────────────┐
//   │ 模块         │ 路径前缀                           │
//   ├──────────────┼────────────────────────────────────┤
//   │ 认证         │ /api/login, /api/auth/*            │
//   │ 用户管理     │ /api/users/*                       │
//   │ 学生管理     │ /api/students/*                    │
//   │ 班级管理     │ /api/classes/*                     │
//   │ 科目管理     │ /api/subjects/*                    │
//   │ 考试管理     │ /api/exams/*                       │
//   │ 成绩管理     │ /api/grades/*                      │
//   │ 统计分析     │ /api/stats/*                       │
//   │ 日志         │ /api/logs                          │
//   └──────────────┴────────────────────────────────────┘
//
// RESTful 风格 API 示例：
//   GET    /api/students          → 获取学生列表
//   POST   /api/students          → 新增学生
//   PUT    /api/students/S2024001 → 修改学号为 S2024001 的学生
//   DELETE /api/students/S2024001 → 删除学号为 S2024001 的学生
//
// 特殊路由（路径中带有子操作）：
//   POST /api/exams/EXAM1/publish  → 发布考试（路径中含 /publish 后缀）
//   POST /api/grades/EXAM1/lock    → 锁定成绩（路径中含 /lock 后缀）
//   GET  /api/grades/student/S2024001 → 查看某学生的全部成绩
// ============================================================
static std::string route(const std::string& method, const std::string& path, const std::string& body,
                          const std::string& queryString, const std::string& token) {

    // ===== 认证模块 =====
    // 登录可以直接访问，不需要 token 验证
    // POST /api/login         ：用户名密码登录，返回 JWT token
    // POST /api/change-password：修改密码（需要 token 验证身份）
    // GET  /api/auth/check    ：验证 token 是否有效
    if (path == "/api/login" && method == "POST") {
        return handlers::login(body);
    }
    if (path == "/api/change-password" && method == "POST") {
        return handlers::changePassword(token, body);
    }
    if (path == "/api/auth/check" && method == "GET") {
        return handlers::checkAuth(token);
    }

    // ===== 用户管理 =====
    // GET    /api/users     → 获取所有用户列表
    // POST   /api/users     → 新增用户
    // PUT    /api/users/{id} → 修改指定用户
    // DELETE /api/users/{id} → 删除指定用户
    if (path == "/api/users" && method == "GET") {
        return handlers::getUsers();
    }
    if (path == "/api/users" && method == "POST") {
        return handlers::addUser(body);
    }
    std::string uid = extractId(path, "/api/users");
    if (!uid.empty() && method == "PUT") {
        return handlers::updateUser(uid, body);
    }
    if (!uid.empty() && method == "DELETE") {
        return handlers::deleteUser(uid);
    }

    // ===== 学生管理 =====
    // GET /api/students?class=高一1班 → 支持按班级筛选
    if (path == "/api/students" && method == "GET") {
        return handlers::getStudents(queryString);
    }
    if (path == "/api/students" && method == "POST") {
        return handlers::addStudent(body);
    }
    std::string sid = extractId(path, "/api/students");
    if (!sid.empty() && method == "PUT") {
        return handlers::updateStudent(sid, body);
    }
    if (!sid.empty() && method == "DELETE") {
        return handlers::deleteStudent(sid);
    }

    // ===== 班级管理 =====
    if (path == "/api/classes" && method == "GET") {
        return handlers::getClasses();
    }
    if (path == "/api/classes" && method == "POST") {
        return handlers::addClass(body);
    }
    std::string cid = extractId(path, "/api/classes");
    if (!cid.empty() && method == "PUT") {
        return handlers::updateClass(cid, body);
    }
    if (!cid.empty() && method == "DELETE") {
        return handlers::deleteClass(cid);
    }

    // ===== 科目管理 =====
    if (path == "/api/subjects" && method == "GET") {
        return handlers::getSubjects();
    }
    if (path == "/api/subjects" && method == "POST") {
        return handlers::addSubject(body);
    }

    // ===== 考试管理 =====
    // 特殊操作：发布（publish）和撤销（retract）考试
    if (path == "/api/exams" && method == "GET") {
        return handlers::getExams(queryString);
    }
    if (path == "/api/exams" && method == "POST") {
        return handlers::addExam(body);
    }
    std::string eid = extractId(path, "/api/exams");
    // 路径中包含 "/publish"，如 /api/exams/EXAM1/publish
    if (eid.find("/publish") != std::string::npos) {
        return handlers::publishExam(eid.substr(0, eid.size() - 8));   // 去掉 "/publish"
    }
    // 路径中包含 "/retract"，如 /api/exams/EXAM1/retract
    if (eid.find("/retract") != std::string::npos) {
        return handlers::retractExam(eid.substr(0, eid.size() - 8));   // 去掉 "/retract"
    }
    if (!eid.empty() && method == "PUT") {
        return handlers::updateExam(eid, body);
    }
    if (!eid.empty() && method == "DELETE") {
        return handlers::deleteExam(eid);
    }

    // ===== 成绩管理 =====
    // GET /api/grades?exam_id=EXAM1 → 按考试筛选成绩
    // POST /api/grades/import        → 批量导入 CSV 成绩
    // GET  /api/grades/export        → 导出成绩为 CSV
    // POST /api/grades/makeup        → 标记补考
    // POST /api/grades/{examId}/lock → 锁定成绩（锁定后不可修改）
    // GET  /api/grades/student/{id}  → 查看指定学生的所有成绩
    if (path == "/api/grades" && method == "GET") {
        return handlers::getGrades(queryString);
    }
    if (path == "/api/grades" && method == "POST") {
        return handlers::setGrades(body);
    }
    if (path == "/api/grades/import" && method == "POST") {
        return handlers::importCSV(body);
    }
    if (path == "/api/grades/export" && method == "GET") {
        return handlers::exportCSV(queryString);
    }
    if (path == "/api/grades/makeup" && method == "POST") {
        return handlers::markMakeup(body);
    }
    std::string gid = extractId(path, "/api/grades");
    if (gid.find("/lock") != std::string::npos) {
        return handlers::lockGrades(gid.substr(0, gid.size() - 5));   // 去掉 "/lock"
    }
    // 前缀匹配路径 /api/grades/student/...
    if (path.find("/api/grades/student/") == 0 && method == "GET") {
        std::string stuid = path.substr(20);   // 跳过 "/api/grades/student/"（20字符）
        return handlers::getStudentGrades(stuid);
    }

    // ===== 统计分析 =====
    // 所有统计接口都在 stats 命名空间中实现，详见 stats.cpp
    // GET /api/stats/rank          → 排名
    // GET /api/stats/class-compare → 班级对比
    // GET /api/stats/distribution  → 分数分布
    // GET /api/stats/warnings      → 不及格预警
    // GET /api/stats/enrollment    → 班级人数统计
    // GET /api/stats/trend/{id}    → 学生成绩趋势
    if (path == "/api/stats/rank" && method == "GET") {
        return stats::getRank(queryString);
    }
    if (path == "/api/stats/class-compare" && method == "GET") {
        return stats::getClassCompare(queryString);
    }
    if (path == "/api/stats/distribution" && method == "GET") {
        return stats::getDistribution(queryString);
    }
    if (path == "/api/stats/warnings" && method == "GET") {
        return stats::getWarnings();
    }
    if (path == "/api/stats/enrollment" && method == "GET") {
        return stats::getEnrollmentStats();
    }
    // 趋势路径：/api/stats/trend/S2024001 → 提取 "S2024001"
    if (path.find("/api/stats/trend/") == 0 && method == "GET") {
        std::string tid = path.substr(17);   // 跳过 "/api/stats/trend/"（17字符）
        return stats::getTrend(tid);
    }

    // ===== 日志 =====
    if (path == "/api/logs" && method == "GET") {
        return handlers::getLogs(queryString);
    }

    // 未匹配任何路由，返回 404 风格的错误响应
    return utils::errorResponse("接口不存在");
}

// ============================================================
// 【程序入口】main 函数
// ============================================================
// 执行流程：
//   1. seedFullData()      → 首次运行时生成测试数据
//   2. GetModuleFileNameA  → 获取 exe 所在目录的绝对路径
//   3. 拼接 webDir 路径     → exe目录\web（前端静态文件目录）
//   4. setRouteHandler     → 注册路由分发函数
//   5. server::start()     → 启动 HTTP 服务器（阻塞，永不返回）
// ============================================================
int main() {
    // 设置控制台编码为 UTF-8，防止 Windows 命令行中文乱码
    // macOS/Linux 终端默认就是 UTF-8，不需要额外设置
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    // 第1步：初始化测试数据（如果 data/ 目录为空）
    seedFullData();

    // 第2步：计算 web 静态文件目录路径
    // 不同操作系统获取程序路径的方式不同：
    //   Windows：GetModuleFileNameA 获取 exe 完整路径
    //   macOS：_NSGetExecutablePath 获取程序路径，realpath 解析符号链接
    //   Linux：readlink 读取 /proc/self/exe 符号链接
    std::string exeDir;
#ifdef _WIN32
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    exeDir = std::string(exePath);
#elif defined(__APPLE__)
    char exePath[PATH_MAX];
    uint32_t bufSize = PATH_MAX;
    if (_NSGetExecutablePath(exePath, &bufSize) == 0) {
        char* resolved = realpath(exePath, NULL);
        exeDir = std::string(resolved ? resolved : exePath);
        if (resolved) free(resolved);
    }
#else  // Linux
    char exePath[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
    if (len != -1) {
        exePath[len] = '\0';
        exeDir = std::string(exePath);
    }
#endif
    // 去掉可执行文件名，只保留目录部分
    size_t lastSlash = exeDir.find_last_of("\\/");
    if (lastSlash != std::string::npos)
        exeDir = exeDir.substr(0, lastSlash);
    // Windows 用反斜杠，macOS/Linux 用正斜杠
    std::string webDir = exeDir + PATH_SEPARATOR + "web";

    // 第3步：注册路由处理函数
    // g_handler 是函数指针，指向 route 函数
    // 后续 server.cpp 中每收到一个 HTTP 请求都会调用它
    server::setRouteHandler(route);

    // 第4步：启动服务器，监听 8080 端口
    // start() 函数包含 while(true) 死循环，这行代码永远不会返回
    server::start(webDir);

    return 0;
}
