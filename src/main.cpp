#include "utils.h"
#include "storage.h"
#include "models.h"
#include "handlers.h"
#include "stats.h"
#include "server.h"
#include <iostream>
#include <string>
#include <vector>
#include <windows.h>

static void seedFullData() {
    utils::ensureDataDir();

    if (utils::fileExists(utils::DATA_DIR + "users.csv") &&
        utils::fileExists(utils::DATA_DIR + "students.csv") &&
        utils::fileExists(utils::DATA_DIR + "exams.csv")) {
        return;
    }

    // ===== 班级 =====
    if (!utils::fileExists(utils::DATA_DIR + "classes.csv")) {
        std::string classNames[9] = {
            "高一1班","高一2班","高一3班",
            "高二1班","高二2班","高二3班",
            "高三1班","高三2班","高三3班"
        };
        std::string grades[9] = {"高一","高一","高一","高二","高二","高二","高三","高三","高三"};
        std::vector<std::vector<std::string>> data;
        data.push_back({"id","name","grade","created_at"});
        for (int i = 0; i < 9; i++) {
            std::vector<std::string> row;
            row.push_back("CLS" + std::to_string(i+1));
            row.push_back(classNames[i]);
            row.push_back(grades[i]);
            row.push_back("2024-09-01");
            data.push_back(row);
        }
        storage::writeCSV("classes.csv", data);
    }

    // ===== 科目 =====
    if (!utils::fileExists(utils::DATA_DIR + "subjects.csv")) {
        std::string subNames[9] = {"语文","数学","英语","物理","化学","生物","历史","政治","地理"};
        int fullScores[9] = {150,150,150,100,100,100,100,100,100};
        std::vector<std::vector<std::string>> data;
        data.push_back({"id","name","full_score"});
        for (int i = 0; i < 9; i++) {
            std::vector<std::string> row;
            row.push_back("SUB" + std::to_string(i+1));
            row.push_back(subNames[i]);
            row.push_back(std::to_string(fullScores[i]));
            data.push_back(row);
        }
        storage::writeCSV("subjects.csv", data);
    }

    // ===== 50个学生 =====
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

    bool needStudents = !utils::fileExists(utils::DATA_DIR + "students.csv");
    bool needUsers = !utils::fileExists(utils::DATA_DIR + "users.csv");

    std::vector<std::vector<std::string>> studentData;
    std::vector<std::vector<std::string>> userData;

    if (needStudents) {
        studentData.push_back({"id","name","birthday","class","gender","enrolled_at"});
    }
    if (needUsers) {
        userData.push_back({"id","name","password","role","phone","created_at"});
        // 管理员
        userData.push_back({"admin","系统管理员",utils::hashPassword("123456"),"admin","","2024-09-01"});
        userData.push_back({"T001","张明老师",utils::hashPassword("123456"),"admin","","2024-09-01"});
        userData.push_back({"T002","李华老师",utils::hashPassword("123456"),"admin","","2024-09-01"});
        userData.push_back({"T003","王芳老师",utils::hashPassword("123456"),"admin","","2024-09-01"});
    }

    for (int i = 0; i < 50; i++) {
        if (needStudents) {
            std::vector<std::string> row;
            for (int j = 0; j < 6; j++) row.push_back(raw[i][j]);
            studentData.push_back(row);
        }
        if (needUsers) {
            userData.push_back({raw[i][0], raw[i][1], utils::hashPassword("123456"), "student", "", "2024-09-01"});
        }
    }

    if (needStudents) storage::writeCSV("students.csv", studentData);
    if (needUsers) storage::writeCSV("users.csv", userData);

    // ===== 考试 =====
    if (!utils::fileExists(utils::DATA_DIR + "exams.csv")) {
        std::vector<std::vector<std::string>> data;
        data.push_back({"id","name","date","status","subjects","weight_config"});
        data.push_back({"EXAM1","高一上学期期中考试","2025-11-10","published",
            "语文|数学|英语|物理|化学",
            "语文:150:1.0|数学:150:1.0|英语:150:1.0|物理:100:0.8|化学:100:0.8"});
        data.push_back({"EXAM2","高一上学期第二次月考","2025-12-20","published",
            "语文|数学|英语|物理|化学|生物|历史",
            "语文:150:1.0|数学:150:1.0|英语:150:1.0|物理:100:0.8|化学:100:0.8|生物:100:0.6|历史:100:0.6"});
        data.push_back({"EXAM3","高一上学期期末考试","2026-01-15","published",
            "语文|数学|英语|物理|化学|生物|历史|政治|地理",
            "语文:150:1.0|数学:150:1.0|英语:150:1.0|物理:100:0.8|化学:100:0.8|生物:100:0.6|历史:100:0.6|政治:100:0.6|地理:100:0.6"});
        storage::writeCSV("exams.csv", data);
    }

    // ===== 模拟成绩 =====
    if (!utils::fileExists(utils::DATA_DIR + "grades.csv")) {
        srand(42);

        std::string examSubs[3] = {
            "语文|数学|英语|物理|化学",
            "语文|数学|英语|物理|化学|生物|历史",
            "语文|数学|英语|物理|化学|生物|历史|政治|地理"
        };

        std::vector<std::vector<std::string>> data;
        data.push_back({"id","student_id","exam_id","scores","is_makeup","submitted_by","submitted_at"});
        int gradeId = 1;

        for (int ei = 0; ei < 3; ei++) {
            std::vector<std::string> subList = utils::split(examSubs[ei], '|');

            for (int si = 0; si < 50; si++) {
                int level = std::stoi(raw[si][6]);
                int basePct[6] = {0, 95, 85, 75, 60, 45};
                std::string scores;
                for (int subi = 0; subi < (int)subList.size(); subi++) {
                    int full = (subList[subi] == "语文" || subList[subi] == "数学" || subList[subi] == "英语") ? 150 : 100;
                    int variation = (si * 7 + subi * 13 + ei * 17) % 21 - 10;
                    int pct = basePct[level] + variation - ei * 3;
                    if (pct > 99) pct = 99;
                    if (pct < 15) pct = 15;
                    if (subi > 0) scores += "|";
                    scores += std::to_string((full * pct) / 100);
                }

                data.push_back({
                    std::to_string(gradeId++),
                    raw[si][0],
                    "EXAM" + std::to_string(ei + 1),
                    scores,
                    "false",
                    "system",
                    utils::getCurrentTime()
                });
            }
        }
        storage::writeCSV("grades.csv", data);
    }

    std::cout << "测试数据初始化完成（50名学生、3场考试、9个班级）" << std::endl;
}

static std::string route(const std::string& path, const std::string& body,
                          const std::string& queryString, const std::string& token) {

    // ===== 认证 =====
    if (path == "/api/login") {
        return handlers::login(body);
    }
    if (path == "/api/change-password") {
        return handlers::changePassword(token, body);
    }
    if (path == "/api/auth/check") {
        return handlers::checkAuth(token);
    }

    // ===== 用户管理 =====
    if (path == "/api/users") {
        return handlers::getUsers();
    }

    // ===== 学生管理 =====
    if (path == "/api/students") {
        return handlers::getStudents(queryString);
    }

    // ===== 班级管理 =====
    if (path == "/api/classes") {
        return handlers::getClasses();
    }

    // ===== 科目管理 =====
    if (path == "/api/subjects") {
        return handlers::getSubjects();
    }

    // ===== 考试管理 =====
    if (path == "/api/exams") {
        return handlers::getExams(queryString);
    }

    // ===== 成绩管理 =====
    if (path == "/api/grades") {
        return handlers::getGrades(queryString);
    }
    if (path == "/api/grades/export") {
        return handlers::exportCSV(queryString);
    }

    // ===== 统计分析 =====
    if (path == "/api/stats/rank") {
        return stats::getRank(queryString);
    }
    if (path == "/api/stats/class-compare") {
        return stats::getClassCompare(queryString);
    }
    if (path == "/api/stats/distribution") {
        return stats::getDistribution(queryString);
    }
    if (path == "/api/stats/warnings") {
        return stats::getWarnings();
    }
    if (path == "/api/stats/enrollment") {
        return stats::getEnrollmentStats();
    }

    // ===== 日志 =====
    if (path == "/api/logs") {
        if (!handlers::isAdmin(token))
            return utils::errorResponse("权限不足");
        return handlers::getLogs(queryString);
    }

    return utils::errorResponse("接口不存在");
}

int main() {
    seedFullData();

    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::string exeDir(exePath);
    size_t lastSlash = exeDir.find_last_of("\\/");
    if (lastSlash != std::string::npos)
        exeDir = exeDir.substr(0, lastSlash);
    std::string webDir = exeDir + "\\web";

    server::setRouteHandler(route);
    server::start(webDir);

    return 0;
}
