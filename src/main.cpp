#include "../lib/httplib.h"
#include "../lib/json.hpp"
#include "util.h"
#include "auth.h"
#include "student.h"
#include "exam.h"
#include "grade.h"
#include "stats.h"
#include "extra.h"
#include "enhance.h"
#include <iostream>
#include <map>
#include <io.h>
#include <direct.h>

using namespace httplib;
using json = nlohmann::json;

static bool fileOrDirExists(const std::string& path) {
    return _access(path.c_str(), 0) == 0;
}

std::string getBody(const Request& req) { return req.body; }
json parseBody(const Request& req) {
    try { return json::parse(req.body); }
    catch (...) { return json::object(); }
}
std::string getParam(const Request& req, const std::string& key) {
    if (req.has_param(key)) return req.get_param_value(key);
    return "";
}
void addCors(Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Headers", "*");
    res.set_header("Access-Control-Allow-Methods", "*");
}

void requireAdmin(const Request& req, Response& res, std::function<void()> next) {
    std::string token = util::extractToken(req.get_header_value("Authorization"));
    json auth = auth::checkAuth(token);
    if (!auth["success"].get<bool>() || auth["data"]["role"] != "admin") {
        res.status = 403;
        res.set_content(util::errorResponse("权限不足").dump(), "application/json");
        return;
    }
    next();
}

void seedData() {
    if (!fileOrDirExists("data")) _mkdir("data");

    if (!fileOrDirExists("data/users.json")) {
        json users = json::array();
        auto addUser = [&](const std::string& id, const std::string& name, const std::string& role) {
            json u;
            u["id"] = id;
            u["name"] = name;
            u["password"] = util::hasher("123456");
            u["role"] = role;
            u["phone"] = "";
            u["created_at"] = util::getCurrentTime();
            users.push_back(u);
        };
        addUser("admin", "系统管理员", "admin");
        addUser("T001", "张老师", "admin");
        addUser("S2024001", "张三", "student");
        addUser("S2024002", "李四", "student");
        addUser("P2024001", "张三家长", "parent");
        util::writeJSON("users.json", users);
    }

    if (!fileOrDirExists("data/students.json")) {
        json students = json::array();
        auto addStu = [&](const std::string& id, const std::string& name, const std::string& cls, const std::string& gender, const std::string& pid) {
            json s;
            s["id"] = id;
            s["name"] = name;
            s["birthday"] = "2008-01-01";
            s["class"] = cls;
            s["gender"] = gender;
            s["parent_id"] = pid;
            s["enrolled_at"] = "2024-09-01";
            students.push_back(s);
        };
        addStu("S2024001", "张三", "高一1班", "男", "P2024001");
        addStu("S2024002", "李四", "高一1班", "女", "");
        addStu("S2024003", "王五", "高一1班", "男", "");
        addStu("S2024004", "赵六", "高一2班", "女", "");
        addStu("S2024005", "孙七", "高一2班", "男", "");
        addStu("S2024006", "周八", "高一2班", "女", "");
        addStu("S2024007", "吴九", "高一3班", "男", "");
        addStu("S2024008", "郑十", "高一3班", "女", "");
        addStu("S2024009", "刘十二", "高一3班", "男", "");
        addStu("S2024010", "陈十三", "高一1班", "女", "");
        addStu("S2024011", "杨十四", "高一2班", "男", "");
        addStu("S2024012", "黄十五", "高一3班", "女", "");
        util::writeJSON("students.json", students);
    }

    if (!fileOrDirExists("data/classes.json")) {
        json classes = json::array();
        for (const char* name : {"高一1班", "高一2班", "高一3班"}) {
            json c;
            c["id"] = std::string("C") + name;
            c["name"] = name;
            c["grade"] = "高一";
            classes.push_back(c);
        }
        util::writeJSON("classes.json", classes);
    }

    if (!fileOrDirExists("data/subjects.json")) {
        json subjects = json::array();
        for (auto& [name, full] : std::map<std::string, int>{
            {"语文", 150}, {"数学", 150}, {"英语", 150},
            {"物理", 100}, {"化学", 100}, {"生物", 100},
            {"历史", 100}, {"政治", 100}, {"地理", 100}}) {
            json s;
            s["id"] = "SUB_" + name;
            s["name"] = name;
            s["full_score"] = full;
            subjects.push_back(s);
        }
        util::writeJSON("subjects.json", subjects);
    }

    if (!fileOrDirExists("data/exams.json")) {
        json exams = json::array();
        json e;
        e["id"] = "EXAM1";
        e["name"] = "高一上学期期中考试";
        e["date"] = "2025-11-10";
        e["subjects"] = {"语文", "数学", "英语", "物理", "化学"};
        e["status"] = "published";
        e["weight_config"]["语文"]["full"] = 150; e["weight_config"]["语文"]["weight"] = 1.0;
        e["weight_config"]["数学"]["full"] = 150; e["weight_config"]["数学"]["weight"] = 1.0;
        e["weight_config"]["英语"]["full"] = 150; e["weight_config"]["英语"]["weight"] = 1.0;
        e["weight_config"]["物理"]["full"] = 100; e["weight_config"]["物理"]["weight"] = 0.8;
        e["weight_config"]["化学"]["full"] = 100; e["weight_config"]["化学"]["weight"] = 0.8;
        exams.push_back(e);
        // Second exam
        json e2;
        e2["id"] = "EXAM2";
        e2["name"] = "高一上学期期末考试";
        e2["date"] = "2026-01-15";
        e2["subjects"] = {"语文","数学","英语","物理","化学","生物","历史","政治"};
        e2["status"] = "published";
        e2["weight_config"]["语文"]["full"] = 150; e2["weight_config"]["语文"]["weight"] = 1.0;
        e2["weight_config"]["数学"]["full"] = 150; e2["weight_config"]["数学"]["weight"] = 1.0;
        e2["weight_config"]["英语"]["full"] = 150; e2["weight_config"]["英语"]["weight"] = 1.0;
        e2["weight_config"]["物理"]["full"] = 100; e2["weight_config"]["物理"]["weight"] = 0.8;
        e2["weight_config"]["化学"]["full"] = 100; e2["weight_config"]["化学"]["weight"] = 0.8;
        e2["weight_config"]["生物"]["full"] = 100; e2["weight_config"]["生物"]["weight"] = 0.7;
        e2["weight_config"]["历史"]["full"] = 100; e2["weight_config"]["历史"]["weight"] = 0.6;
        e2["weight_config"]["政治"]["full"] = 100; e2["weight_config"]["政治"]["weight"] = 0.6;
        exams.push_back(e2);
        util::writeJSON("exams.json", exams);
    }

    if (!fileOrDirExists("data/grades.json")) {
        json grades = json::array();
        auto addGrade = [&](const std::string& sid, const std::string& eid, const json& scores) {
            json g;
            g["student_id"] = sid;
            g["exam_id"] = eid;
            g["scores"] = scores;
            g["comments"] = json::object();
            g["is_makeup"] = false;
            g["submitted_by"] = "admin";
            g["submitted_at"] = util::getCurrentTime();
            grades.push_back(g);
        };
        json s1 = {{"语文", "120"}, {"数学", "135"}, {"英语", "110"}, {"物理", "85"}, {"化学", "90"}};
        json s2 = {{"语文", "105"}, {"数学", "142"}, {"英语", "125"}, {"物理", "78"}, {"化学", "82"}};
        json s3 = {{"语文", "98"}, {"数学", "88"}, {"英语", "105"}, {"物理", "92"}, {"化学", "76"}};
        json s4 = {{"语文", "115"}, {"数学", "120"}, {"英语", "118"}, {"物理", "70"}, {"化学", "88"}};
        json s5 = {{"语文", "130"}, {"数学", "95"}, {"英语", "100"}, {"物理", "88"}, {"化学", "95"}};
        json s6 = {{"语文", "108"}, {"数学", "115"}, {"英语", "90"}, {"物理", "65"}, {"化学", "70"}};
        json s7 = {{"语文", "122"}, {"数学", "130"}, {"英语", "115"}, {"物理", "80"}, {"化学", "85"}};
        json s8 = {{"语文", "95"}, {"数学", "78"}, {"英语", "88"}, {"物理", "75"}, {"化学", "65"}};
        addGrade("S2024001", "EXAM1", s1);
        addGrade("S2024002", "EXAM1", s2);
        addGrade("S2024003", "EXAM1", s3);
        addGrade("S2024004", "EXAM1", s4);
        addGrade("S2024005", "EXAM1", s5);
        addGrade("S2024006", "EXAM1", s6);
        addGrade("S2024007", "EXAM1", s7);
        addGrade("S2024008", "EXAM1", s8);
        // EXAM2 grades
        json e2_s1 = {{"语文","125"},{"数学","138"},{"英语","115"},{"物理","88"},{"化学","92"},{"生物","78"},{"历史","72"},{"政治","80"}};
        json e2_s2 = {{"语文","108"},{"数学","145"},{"英语","128"},{"物理","80"},{"化学","85"},{"生物","82"},{"历史","65"},{"政治","70"}};
        json e2_s3 = {{"语文","102"},{"数学","92"},{"英语","108"},{"物理","95"},{"化学","78"},{"生物","88"},{"历史","75"},{"政治","72"}};
        json e2_s4 = {{"语文","118"},{"数学","122"},{"英语","120"},{"物理","72"},{"化学","90"},{"生物","70"},{"历史","68"},{"政治","75"}};
        json e2_s5 = {{"语文","135"},{"数学","98"},{"英语","105"},{"物理","90"},{"化学","96"},{"生物","85"},{"历史","78"},{"政治","82"}};
        json e2_s6 = {{"语文","110"},{"数学","118"},{"英语","95"},{"物理","68"},{"化学","72"},{"生物","65"},{"历史","70"},{"政治","65"}};
        json e2_s7 = {{"语文","128"},{"数学","135"},{"英语","118"},{"物理","82"},{"化学","88"},{"生物","80"},{"历史","72"},{"政治","76"}};
        json e2_s8 = {{"语文","98"},{"数学","82"},{"英语","90"},{"物理","78"},{"化学","68"},{"生物","72"},{"历史","60"},{"政治","62"}};
        addGrade("S2024001","EXAM2",e2_s1);
        addGrade("S2024002","EXAM2",e2_s2);
        addGrade("S2024003","EXAM2",e2_s3);
        addGrade("S2024004","EXAM2",e2_s4);
        addGrade("S2024005","EXAM2",e2_s5);
        addGrade("S2024006","EXAM2",e2_s6);
        addGrade("S2024007","EXAM2",e2_s7);
        addGrade("S2024008","EXAM2",e2_s8);
        util::writeJSON("grades.json", grades);
    }

    if (!fileOrDirExists("data/errors.json")) {
        json errors = json::array();
        auto addErr = [&](const std::string& sid, const std::string& eid, const std::string& sub, const std::string& q, const std::string& wa, const std::string& ca, const std::string& et, const std::string& kp) {
            json err;
            err["student_id"] = sid; err["exam_id"] = eid; err["subject"] = sub;
            err["question"] = q; err["wrong_answer"] = wa; err["correct_answer"] = ca;
            err["error_type"] = et; err["knowledge_point"] = kp;
            err["created_at"] = util::getCurrentTime();
            errors.push_back(err);
        };
        addErr("S2024001","EXAM1","数学","第22题 函数最值", "f(x)=x^2-2x", "f(x)=x^2-4x+4", "计算错误", "二次函数最值问题");
        addErr("S2024001","EXAM1","英语","完形填空 第35题", "was", "were", "概念不清", "虚拟语气");
        addErr("S2024001","EXAM1","物理","第15题 力学", "F=mg", "F=2mg", "审题失误", "受力分析");
        addErr("S2024002","EXAM1","语文","阅读理解 第8题", "表达了思乡之情", "表达了壮志未酬的悲愤", "概念不清", "诗歌鉴赏-情感分析");
        addErr("S2024002","EXAM1","物理","第14题 电路", "I=0.5A", "I=1A", "计算错误", "并联电路电流计算");
        addErr("S2024003","EXAM1","数学","第18题 三角函数", "sin30°=0.5", "sin30°=0.5 无错误", "概念不清", "特殊角三角函数值");
        addErr("S2024003","EXAM1","化学","第10题 化学方程式", "Na+H2O=NaOH", "2Na+2H2O=2NaOH+H2", "粗心大意", "化学方程式配平");
        addErr("S2024003","EXAM1","英语","语法填空 第42题", "go", "going", "知识遗忘", "动名词作宾语");
        addErr("S2024004","EXAM1","数学","第20题 概率", "1/3", "2/3", "方法不当", "条件概率计算");
        addErr("S2024004","EXAM1","化学","第8题 离子反应", "Cu2+", "Fe3+", "概念不清", "离子共存判断");
        addErr("S2024005","EXAM1","语文","作文 立意", "跑题", "紧扣材料中心", "审题失误", "材料作文审题");
        addErr("S2024005","EXAM1","数学","第16题 解析几何", "y=x", "y=2x+1", "计算错误", "直线方程求解");
        addErr("S2024006","EXAM1","物理","第12题 热学", "Q=cmΔt", "无错误", "方法不当", "热学计算");
        addErr("S2024006","EXAM1","英语","阅读理解C篇 第28题", "C", "A", "审题失误", "事实细节题");
        addErr("S2024006","EXAM1","化学","第6题 有机物", "乙醇", "乙酸", "知识遗忘", "有机物性质");
        addErr("S2024006","EXAM1","物理","第17题 电磁", "B=0.5T", "B=1T", "计算错误", "安培力计算");
        addErr("S2024007","EXAM1","数学","第21题 导数", "f'(x)=2x", "f'(x)=3x^2-2", "方法不当", "导数运算法则");
        addErr("S2024007","EXAM1","英语","语法填空 第45题", "has", "have", "粗心大意", "主谓一致");
        addErr("S2024008","EXAM1","数学","第5题 集合", "{1,2}", "{1,2,3}", "概念不清", "集合运算");
        addErr("S2024008","EXAM1","语文","古诗词默写 第3题", "错误默写", "大漠孤烟直长河落日圆", "知识遗忘", "古诗词默写-王维");
        addErr("S2024008","EXAM1","化学","第12题 实验操作", "先加酸", "先加碱", "方法不当", "酸碱中和滴定");
        addErr("S2024008","EXAM1","物理","第19题 动量", "mv", "m1v1+m2v2=m1v1'+m2v2'", "知识遗忘", "动量守恒定律");
        util::writeJSON("errors.json", errors);
    }

    std::cout << "Data seeding complete." << std::endl;
}

int main() {
    seedData();
    Server svr;

    svr.Options(".*", [](const Request&, Response& res) {
        addCors(res);
        res.status = 204;
    });

    svr.Post("/api/login", [](const Request& req, Response& res) {
        addCors(res);
        json body = parseBody(req);
        std::string username = body.value("username", "");
        std::string password = body.value("password", "");
        res.set_content(auth::login(username, password).dump(), "application/json");
    });

    svr.Post("/api/register", [](const Request& req, Response& res) {
        addCors(res);
        json body = parseBody(req);
        std::string username = body.value("username", "");
        std::string password = body.value("password", "");
        std::string name = body.value("name", "");
        std::string role = body.value("role", "student");
        if (username.empty() || password.empty() || name.empty()) {
            res.set_content(util::errorResponse("用户名、密码和姓名不能为空").dump(), "application/json");
            return;
        }
        if (password.length() < 4) {
            res.set_content(util::errorResponse("密码至少4位").dump(), "application/json");
            return;
        }
        json users = util::readJSON("users.json");
        for (auto& u : users) {
            if (u["id"] == username) {
                res.set_content(util::errorResponse("该用户名已存在").dump(), "application/json");
                return;
            }
        }
        json newUser;
        newUser["id"] = username;
        newUser["name"] = name;
        newUser["password"] = util::hasher(password);
        newUser["role"] = role;
        newUser["phone"] = body.value("phone", "");
        newUser["created_at"] = util::getCurrentTime();
        users.push_back(newUser);
        util::writeJSON("users.json", users);

        if (role == "student") {
            json students = util::readJSON("students.json");
            json s;
            s["id"] = username;
            s["name"] = name;
            s["birthday"] = "";
            s["class"] = body.value("class", "");
            s["gender"] = body.value("gender", "");
            s["parent_id"] = body.value("parent_id", "");
            s["enrolled_at"] = util::getCurrentTime();
            students.push_back(s);
            util::writeJSON("students.json", students);
        }

        util::logAction(username, "注册", "新用户注册: " + name);
        res.set_content(util::jsonResponse(true, "注册成功", newUser).dump(), "application/json");
    });

    svr.Post("/api/change-password", [](const Request& req, Response& res) {
        addCors(res);
        std::string token = util::extractToken(req.get_header_value("Authorization"));
        json body = parseBody(req);
        res.set_content(auth::changePassword(token,
            body.value("old_password", ""),
            body.value("new_password", "")).dump(), "application/json");
    });

    svr.Get("/api/auth/check", [](const Request& req, Response& res) {
        addCors(res);
        std::string token = util::extractToken(req.get_header_value("Authorization"));
        res.set_content(auth::checkAuth(token).dump(), "application/json");
    });

    // Users
    svr.Get("/api/users", [](const Request& req, Response& res) {
        addCors(res);
        requireAdmin(req, res, [&]() {
            res.set_content(auth::getUsers().dump(), "application/json");
        });
    });

    svr.Post("/api/users", [](const Request& req, Response& res) {
        addCors(res);
        requireAdmin(req, res, [&]() {
            json body = parseBody(req);
            res.set_content(auth::addUser(body).dump(), "application/json");
        });
    });

    svr.Delete(R"(/api/users/(.*))", [](const Request& req, Response& res) {
        addCors(res);
        requireAdmin(req, res, [&]() {
            res.set_content(auth::deleteUser(req.matches[1]).dump(), "application/json");
        });
    });

    svr.Put(R"(/api/users/(.*))", [](const Request& req, Response& res) {
        addCors(res);
        requireAdmin(req, res, [&]() {
            json body = parseBody(req);
            res.set_content(auth::updateUser(req.matches[1], body).dump(), "application/json");
        });
    });

    // Students
    svr.Get("/api/students", [](const Request& req, Response& res) {
        addCors(res);
        int page = std::max(1, std::stoi(getParam(req, "page").empty() ? "1" : getParam(req, "page")));
        int size = std::max(1, std::stoi(getParam(req, "size").empty() ? "20" : getParam(req, "size")));
        res.set_content(student::getStudents(getParam(req, "search"), getParam(req, "class"), page, size).dump(), "application/json");
    });

    svr.Post("/api/students", [](const Request& req, Response& res) {
        addCors(res);
        requireAdmin(req, res, [&]() {
            res.set_content(student::addStudent(parseBody(req)).dump(), "application/json");
        });
    });

    svr.Put(R"(/api/students/(.*))", [](const Request& req, Response& res) {
        addCors(res);
        requireAdmin(req, res, [&]() {
            res.set_content(student::updateStudent(req.matches[1], parseBody(req)).dump(), "application/json");
        });
    });

    svr.Delete(R"(/api/students/(.*))", [](const Request& req, Response& res) {
        addCors(res);
        requireAdmin(req, res, [&]() {
            res.set_content(student::deleteStudent(req.matches[1]).dump(), "application/json");
        });
    });

    // Classes
    svr.Get("/api/classes", [](const Request& req, Response& res) {
        addCors(res);
        res.set_content(student::getClasses().dump(), "application/json");
    });

    svr.Post("/api/classes", [](const Request& req, Response& res) {
        addCors(res);
        requireAdmin(req, res, [&]() {
            res.set_content(student::addClass(parseBody(req)).dump(), "application/json");
        });
    });

    svr.Put(R"(/api/classes/(.*))", [](const Request& req, Response& res) {
        addCors(res);
        requireAdmin(req, res, [&]() {
            res.set_content(student::updateClass(req.matches[1], parseBody(req)).dump(), "application/json");
        });
    });

    svr.Delete(R"(/api/classes/(.*))", [](const Request& req, Response& res) {
        addCors(res);
        requireAdmin(req, res, [&]() {
            res.set_content(student::deleteClass(req.matches[1]).dump(), "application/json");
        });
    });

    // Subjects
    svr.Get("/api/subjects", [](const Request& req, Response& res) {
        addCors(res);
        res.set_content(student::getSubjects().dump(), "application/json");
    });

    svr.Post("/api/subjects", [](const Request& req, Response& res) {
        addCors(res);
        requireAdmin(req, res, [&]() {
            res.set_content(student::addSubject(parseBody(req)).dump(), "application/json");
        });
    });

    // Enrollment Stats
    svr.Get("/api/stats/enrollment", [](const Request& req, Response& res) {
        addCors(res);
        res.set_content(student::getEnrollmentStats().dump(), "application/json");
    });

    // Exams
    svr.Get("/api/exams", [](const Request& req, Response& res) {
        addCors(res);
        res.set_content(exam::getExams(getParam(req, "status")).dump(), "application/json");
    });

    svr.Post("/api/exams", [](const Request& req, Response& res) {
        addCors(res);
        requireAdmin(req, res, [&]() {
            res.set_content(exam::addExam(parseBody(req)).dump(), "application/json");
        });
    });

    svr.Put(R"(/api/exams/(.*))", [](const Request& req, Response& res) {
        addCors(res);
        requireAdmin(req, res, [&]() {
            res.set_content(exam::updateExam(req.matches[1], parseBody(req)).dump(), "application/json");
        });
    });

    svr.Delete(R"(/api/exams/(.*))", [](const Request& req, Response& res) {
        addCors(res);
        requireAdmin(req, res, [&]() {
            res.set_content(exam::deleteExam(req.matches[1]).dump(), "application/json");
        });
    });

    svr.Put(R"(/api/exams/(.*)/publish)", [](const Request& req, Response& res) {
        addCors(res);
        requireAdmin(req, res, [&]() {
            res.set_content(exam::publishExam(req.matches[1]).dump(), "application/json");
        });
    });

    svr.Put(R"(/api/exams/(.*)/retract)", [](const Request& req, Response& res) {
        addCors(res);
        requireAdmin(req, res, [&]() {
            res.set_content(exam::retractExam(req.matches[1]).dump(), "application/json");
        });
    });

    // Grades
    svr.Post("/api/grades", [](const Request& req, Response& res) {
        addCors(res);
        requireAdmin(req, res, [&]() {
            res.set_content(grade::setGrades(parseBody(req)).dump(), "application/json");
        });
    });

    svr.Post("/api/grades/import", [](const Request& req, Response& res) {
        addCors(res);
        requireAdmin(req, res, [&]() {
            json body = parseBody(req);
            res.set_content(grade::importCSV(body.value("csv", ""), body.value("exam_id", "")).dump(), "application/json");
        });
    });

    svr.Get("/api/grades/export", [](const Request& req, Response& res) {
        addCors(res);
        res.set_content(grade::exportCSV(getParam(req, "exam_id")).dump(), "application/json");
    });

    svr.Put(R"(/api/grades/(.*)/lock)", [](const Request& req, Response& res) {
        addCors(res);
        requireAdmin(req, res, [&]() {
            res.set_content(grade::lockGrades(req.matches[1]).dump(), "application/json");
        });
    });

    svr.Get("/api/grades", [](const Request& req, Response& res) {
        addCors(res);
        res.set_content(grade::getGrades(getParam(req, "exam_id"), getParam(req, "student_id")).dump(), "application/json");
    });

    svr.Get(R"(/api/grades/student/(.*))", [](const Request& req, Response& res) {
        addCors(res);
        res.set_content(grade::getStudentGrades(req.matches[1]).dump(), "application/json");
    });

    // Makeup
    svr.Post("/api/grades/makeup", [](const Request& req, Response& res) {
        addCors(res);
        requireAdmin(req, res, [&]() {
            json body = parseBody(req);
            res.set_content(grade::markMakeup(
                body.value("student_id", ""),
                body.value("exam_id", ""),
                body.value("scores", json::object())
            ).dump(), "application/json");
        });
    });

    // Stats
    svr.Get("/api/stats/rank", [](const Request& req, Response& res) {
        addCors(res);
        res.set_content(stats::getRank(
            getParam(req, "exam_id"),
            getParam(req, "type"),
            getParam(req, "subject")
        ).dump(), "application/json");
    });

    svr.Get("/api/stats/class-compare", [](const Request& req, Response& res) {
        addCors(res);
        res.set_content(stats::getClassCompare(getParam(req, "exam_id")).dump(), "application/json");
    });

    svr.Get("/api/stats/distribution", [](const Request& req, Response& res) {
        addCors(res);
        res.set_content(stats::getDistribution(getParam(req, "exam_id"), getParam(req, "subject")).dump(), "application/json");
    });

    svr.Get(R"(/api/stats/trend/(.*))", [](const Request& req, Response& res) {
        addCors(res);
        res.set_content(stats::getTrend(req.matches[1]).dump(), "application/json");
    });

    svr.Get("/api/stats/warnings", [](const Request& req, Response& res) {
        addCors(res);
        res.set_content(stats::getWarnings().dump(), "application/json");
    });

    // Logs
    svr.Get("/api/logs", [](const Request& req, Response& res) {
        addCors(res);
        requireAdmin(req, res, [&]() {
            int page = std::max(1, std::stoi(getParam(req, "page").empty() ? "1" : getParam(req, "page")));
            int size = std::max(1, std::stoi(getParam(req, "size").empty() ? "50" : getParam(req, "size")));
            res.set_content(util::getLogs(page, size, getParam(req, "user")).dump(), "application/json");
        });
    });

    // Error Notebook
    svr.Post("/api/errors", [](const Request& req, Response& res) {
        addCors(res);
        requireAdmin(req, res, [&]() {
            res.set_content(extra::setError(parseBody(req)).dump(), "application/json");
        });
    });

    svr.Get("/api/errors", [](const Request& req, Response& res) {
        addCors(res);
        int page = std::max(1, std::stoi(getParam(req, "page").empty() ? "1" : getParam(req, "page")));
        int size = std::max(1, std::stoi(getParam(req, "size").empty() ? "20" : getParam(req, "size")));
        res.set_content(extra::getErrors(
            getParam(req, "student_id"),
            getParam(req, "exam_id"),
            getParam(req, "subject"),
            page, size
        ).dump(), "application/json");
    });

    svr.Get("/api/errors/stats", [](const Request& req, Response& res) {
        addCors(res);
        res.set_content(extra::getErrorStats(getParam(req, "student_id")).dump(), "application/json");
    });

    svr.Get("/api/stats/imbalance", [](const Request& req, Response& res) {
        addCors(res);
        res.set_content(extra::getImbalance(getParam(req, "student_id")).dump(), "application/json");
    });

    svr.Get("/api/stats/ability", [](const Request& req, Response& res) {
        addCors(res);
        res.set_content(extra::getAbilityMap(getParam(req, "student_id"), getParam(req, "exam_id")).dump(), "application/json");
    });

    svr.Get("/api/report", [](const Request& req, Response& res) {
        addCors(res);
        res.set_content(extra::getReport(getParam(req, "student_id")).dump(), "application/json");
    });

    svr.Get("/api/suggest", [](const Request& req, Response& res) {
        addCors(res);
        res.set_content(extra::getSuggest(getParam(req, "student_id")).dump(), "application/json");
    });

    // Goals
    svr.Post("/api/goals", [](const Request& req, Response& res) {
        addCors(res);
        res.set_content(enhance::setGoal(parseBody(req)).dump(), "application/json");
    });
    svr.Get("/api/goals", [](const Request& req, Response& res) {
        addCors(res);
        res.set_content(enhance::getGoals(getParam(req, "student_id")).dump(), "application/json");
    });
    // Knowledge map
    svr.Get("/api/knowledge-map", [](const Request& req, Response& res) {
        addCors(res);
        res.set_content(enhance::getKnowledgeMap(getParam(req, "student_id")).dump(), "application/json");
    });
    // Portfolio
    svr.Get("/api/portfolio", [](const Request& req, Response& res) {
        addCors(res);
        res.set_content(enhance::getPortfolio(getParam(req, "student_id")).dump(), "application/json");
    });
    // Points
    svr.Get("/api/points", [](const Request& req, Response& res) {
        addCors(res);
        res.set_content(enhance::getPoints(getParam(req, "student_id")).dump(), "application/json");
    });
    svr.Post("/api/points/add", [](const Request& req, Response& res) {
        addCors(res);
        json body = parseBody(req);
        res.set_content(enhance::addPoints(
            body.value("student_id", ""),
            body.value("points", 0),
            body.value("reason", "")
        ).dump(), "application/json");
    });
    // Review plan
    svr.Get("/api/review-plan", [](const Request& req, Response& res) {
        addCors(res);
        res.set_content(enhance::getReviewPlan(getParam(req, "student_id")).dump(), "application/json");
    });
    // Check-in
    svr.Post("/api/checkin", [](const Request& req, Response& res) {
        addCors(res);
        res.set_content(enhance::checkin(parseBody(req)).dump(), "application/json");
    });
    svr.Get("/api/checkins", [](const Request& req, Response& res) {
        addCors(res);
        res.set_content(enhance::getCheckins(getParam(req, "student_id")).dump(), "application/json");
    });

    // Serve static files
    svr.set_mount_point("/", "./web");

    std::cout << "EduGrade Server started on http://localhost:8080" << std::endl;
    svr.listen("0.0.0.0", 8080);

    return 0;
}
