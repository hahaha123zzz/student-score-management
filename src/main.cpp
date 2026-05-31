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
#ifdef _WIN32
#include <io.h>
#include <direct.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdlib.h>
#include <stdint.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

using namespace httplib;
using json = nlohmann::json;

static bool fileOrDirExists(const std::string& path) {
#ifdef _WIN32
    return _access(path.c_str(), 0) == 0;
#else
    return access(path.c_str(), F_OK) == 0;
#endif
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
    if (!fileOrDirExists("data")) {
#ifdef _WIN32
        _mkdir("data");
#else
        mkdir("data", 0755);
#endif
    }

    std::vector<std::string> allStuIds;
    std::vector<std::string> allStuNames;
    std::vector<std::string> allStuClasses;
    std::vector<std::string> allStuGenders;
    std::vector<int> allStuLevels;
    bool studentsBuilt = false;

    // ========== CLASSES ==========
    std::vector<std::string> allClasses = {
        "高一1班","高一2班","高一3班",
        "高二1班","高二2班","高二3班",
        "高三1班","高三2班","高三3班"
    };
    std::vector<std::string> grades = {"高一","高一","高一","高二","高二","高二","高三","高三","高三"};

    if (!fileOrDirExists("data/classes.json")) {
        json classes = json::array();
        for (size_t i = 0; i < allClasses.size(); i++) {
            json c;
            c["id"] = "CLS" + std::to_string(i+1);
            c["name"] = allClasses[i];
            c["grade"] = grades[i];
            c["created_at"] = "2024-09-01";
            classes.push_back(c);
        }
        util::writeJSON("classes.json", classes);
    }

    // ========== SUBJECTS ==========
    if (!fileOrDirExists("data/subjects.json")) {
        json subjects = json::array();
        std::vector<std::pair<std::string,int>> subjs = {
            {"语文",150},{"数学",150},{"英语",150},
            {"物理",100},{"化学",100},{"生物",100},
            {"历史",100},{"政治",100},{"地理",100}
        };
        for (size_t i = 0; i < subjs.size(); i++) {
            json s;
            s["id"] = "SUB" + std::to_string(i+1);
            s["name"] = subjs[i].first;
            s["full_score"] = subjs[i].second;
            subjects.push_back(s);
        }
        util::writeJSON("subjects.json", subjects);
    }

    // ========== STUDENTS (50 students across 9 classes) ==========
    std::vector<std::vector<std::string>> classStudents = {
        {"S2024001","01","张三","男", "S2024002","02","李四","女", "S2024003","03","王明","男", "S2024004","04","陈丽","女", "S2024005","05","刘洋","男", "S2024006","01","赵敏","女"},
        {"S2024007","02","孙伟","男", "S2024008","03","周芳","女", "S2024009","01","吴强","男", "S2024010","02","郑婷","女", "S2024011","03","冯军","男", "S2024012","04","何静","女"},
        {"S2024013","05","沈浩","男", "S2024014","01","韩雪","女", "S2024015","02","秦勇","男", "S2024016","03","许莉","女", "S2024017","01","邓超","男", "S2024018","02","彭蕾","女"},
        {"S2025001","01","马飞","男", "S2025002","02","宋婷","女", "S2025003","03","唐龙","男", "S2025004","04","曹娜","女", "S2025005","05","高翔","男"},
        {"S2025006","01","罗琳","女", "S2025007","02","梁志","男", "S2025008","03","谢慧","女", "S2025009","04","黄磊","男", "S2025010","05","朱玥","女"},
        {"S2025011","01","徐明","男", "S2025012","02","胡燕","女", "S2025013","03","林峰","男", "S2025014","04","郭丽","女", "S2025015","05","蔡刚","男"},
        {"S2026001","01","潘玉","女", "S2026002","02","蒋文","男", "S2026003","03","余磊","男", "S2026004","04","苏芳","女", "S2026005","05","叶波","男"},
        {"S2026006","01","姜涛","男", "S2026007","02","董兰","女", "S2026008","03","程浩","男", "S2026009","04","魏敏","女"},
        {"S2026010","01","袁帅","男", "S2026011","02","谌洁","女", "S2026012","03","欧阳","女", "S2026013","04","段鹏","男", "S2026014","05","靳悦","女"}
    };

    // Each entry: [id, levelStr, name, gender, id, levelStr, name, gender, ...]
    // level: "01"=excellent, "02"=good, "03"=average, "04"=below avg, "05"=weak

    for (size_t ci = 0; ci < classStudents.size(); ci++) {
        for (size_t si = 0; si < classStudents[ci].size(); si+=4) {
            std::string id = classStudents[ci][si];
            std::string name = classStudents[ci][si+2];
            std::string gender = classStudents[ci][si+3];
            int lvl = std::stoi(classStudents[ci][si+1]);
            allStuIds.push_back(id);
            allStuNames.push_back(name);
            allStuClasses.push_back(allClasses[ci]);
            allStuGenders.push_back(gender);
            allStuLevels.push_back(lvl);
        }
    }

    // ========== USERS ==========
    if (!fileOrDirExists("data/users.json")) {
        json users = json::array();
        auto addUsr = [&](const std::string& id, const std::string& name, const std::string& role) {
            json u;
            u["id"] = id; u["name"] = name; u["role"] = role;
            u["password"] = util::hasher("123456");
            u["phone"] = ""; u["created_at"] = util::getCurrentTime();
            users.push_back(u);
        };
        addUsr("admin","系统管理员","admin");
        addUsr("T001","张明老师","admin");
        addUsr("T002","李华老师","admin");
        addUsr("T003","王芳老师","admin");
        for (size_t i = 0; i < allStuIds.size(); i++)
            addUsr(allStuIds[i], allStuNames[i], "student");
        // Parents for some students
        for (size_t i = 0; i < 6; i++) {
            std::string pid = "P2024" + std::string(1,'0'+i+1);
            addUsr(pid, allStuNames[i]+"家长", "parent");
        }
        util::writeJSON("users.json", users);
    }

    // ========== STUDENTS ==========
    if (!fileOrDirExists("data/students.json")) {
        json students = json::array();
        for (size_t i = 0; i < allStuIds.size(); i++) {
            json s;
            s["id"] = allStuIds[i];
            s["name"] = allStuNames[i];
            s["birthday"] = std::to_string(2007 + (i%3)) + "-0" + std::to_string(1+(i%9)) + "-" + std::to_string(10+(i%20));
            s["class"] = allStuClasses[i];
            s["gender"] = allStuGenders[i];
            s["parent_id"] = (i < 6) ? ("P2024"+std::to_string(i+1)) : "";
            s["enrolled_at"] = "2024-09-01";
            students.push_back(s);
        }
        util::writeJSON("students.json", students);
    }

    // ========== EXAMS ==========
    if (!fileOrDirExists("data/exams.json")) {
        json exams = json::array();
        struct ExamDef { std::string id,name,date; std::vector<std::string> subs; };
        std::vector<ExamDef> examDefs = {
            {"EXAM1","高一上学期期中考试","2025-11-10",{"语文","数学","英语","物理","化学"}},
            {"EXAM2","高一上学期第二次月考","2025-12-20",{"语文","数学","英语","物理","化学","生物","历史"}},
            {"EXAM3","高一上学期期末考试","2026-01-15",{"语文","数学","英语","物理","化学","生物","历史","政治","地理"}}
        };
        std::vector<int> fullScores = {150,150,150,100,100,100,100,100,100};
        for (auto& ed : examDefs) {
            json e;
            e["id"] = ed.id;
            e["name"] = ed.name;
            e["date"] = ed.date;
            e["subjects"] = ed.subs;
            e["status"] = "published";
            e["created_at"] = util::getCurrentTime();
            for (size_t si = 0; si < ed.subs.size(); si++) {
                std::string sub = ed.subs[si];
                int full = 100;
                for (size_t fi = 0; fi < fullScores.size(); fi++) {
                    if (fi < ed.subs.size()) {
                        if (sub == "语文" || sub == "数学" || sub == "英语") full = 150;
                        else full = 100;
                    }
                }
                double weight = (sub=="语文"||sub=="数学"||sub=="英语")?1.0:(sub=="物理"||sub=="化学")?0.8:0.6;
                e["weight_config"][sub]["full"] = full;
                e["weight_config"][sub]["weight"] = weight;
            }
            exams.push_back(e);
        }
        util::writeJSON("exams.json", exams);
    }

    // ========== GRADES (generated with ability-based variation) ==========
    if (!fileOrDirExists("data/grades.json")) {
        json grades = json::array();
        auto addGr = [&](const std::string& sid, const std::string& eid, const json& scores) {
            json g;
            g["student_id"] = sid; g["exam_id"] = eid;
            g["scores"] = scores; g["comments"] = json::object();
            g["is_makeup"] = false; g["submitted_by"] = "admin";
            g["submitted_at"] = util::getCurrentTime();
            grades.push_back(g);
        };
        // Score formula: base depends on ability level (1=excellent ~85-100%, 5=weak ~30-60%)
        // plus subject-specific variation and exam difficulty variation
        auto genScore = [](int level, int full, int variation, int difficulty) -> std::string {
            // level 1->90-100% range, level 2->80-95%, level 3->65-85%, level 4->50-75%, level 5->30-60%
            int basePct[] = {95, 85, 75, 60, 45}; // midpoints for each level
            int range2[] = {12, 16, 18, 22, 28}; // variation range per level
            int pct = basePct[level-1] + (variation % range2[level-1]) - (range2[level-1]/2) - difficulty * 3;
            if (pct > 99) pct = 99;
            if (pct < 15) pct = 15;
            int score = (full * pct) / 100;
            return std::to_string(score);
        };

        for (size_t ei = 0; ei < 3; ei++) {
            std::string eid = "EXAM" + std::to_string(ei+1);
            int difficulty = (ei == 0) ? 0 : (ei == 1) ? 1 : 2; // exams get harder
            // For exam 1 (期中): 5 subjects; exam2 (月考): 7; exam3 (期末): 9
            std::vector<std::string> subs = (ei==0)?
                std::vector<std::string>{"语文","数学","英语","物理","化学"}:
                (ei==1)?
                std::vector<std::string>{"语文","数学","英语","物理","化学","生物","历史"}:
                std::vector<std::string>{"语文","数学","英语","物理","化学","生物","历史","政治","地理"};

            for (size_t si = 0; si < allStuIds.size(); si++) {
                json scores;
                for (size_t subi = 0; subi < subs.size(); subi++) {
                    int full = (subs[subi]=="语文"||subs[subi]=="数学"||subs[subi]=="英语")?150:100;
                    int variation = (si * 7 + subi * 13 + ei * 17) % 101; // deterministic "random"
                    // Slightly better scores on 期末 (studying effect)
                    int adj = allStuLevels[si];
                    if (ei == 2) adj = std::max(1, adj-1); // improvement over year
                    scores[subs[subi]] = genScore(adj, full, variation, difficulty);
                }
                addGr(allStuIds[si], eid, scores);
            }
        }
        util::writeJSON("grades.json", grades);
    }

    // ========== ERRORS (80+ records) ==========
    if (!fileOrDirExists("data/errors.json")) {
        json errors = json::array();
        std::vector<std::string> errTypes = {"概念不清","计算错误","审题失误","知识遗忘","方法不当","粗心大意"};
        auto addErr = [&](const std::string& sid, const std::string& eid, const std::string& sub,
                          const std::string& q, const std::string& wa, const std::string& ca,
                          const std::string& et, const std::string& kp) {
            json err;
            err["student_id"]=sid; err["exam_id"]=eid; err["subject"]=sub;
            err["question"]=q; err["wrong_answer"]=wa; err["correct_answer"]=ca;
            err["error_type"]=et; err["knowledge_point"]=kp;
            err["created_at"]=util::getCurrentTime();
            errors.push_back(err);
        };
        // Generate errors: for each student with level >=3 (weaker students), pick 2-5 errors per exam
        for (size_t ei = 0; ei < 3; ei++) {
            std::string eid = "EXAM" + std::to_string(ei+1);
            std::vector<std::string> subs = (ei==0)?
                std::vector<std::string>{"语文","数学","英语","物理","化学"}:
                (ei==1)?
                std::vector<std::string>{"语文","数学","英语","物理","化学","生物","历史"}:
                std::vector<std::string>{"语文","数学","英语","物理","化学","生物","历史","政治","地理"};
            for (size_t si = 0; si < allStuIds.size(); si++) {
                int lvl = allStuLevels[si];
                int errCount = (lvl <= 2) ? (lvl+ (si%2)) : (lvl + (si%3)); // more errors for weaker students
                errCount = errCount > 5 ? 5 : errCount;
                for (int ec = 0; ec < errCount; ec++) {
                    int subi = (si + ec + ei) % subs.size();
                    std::string sub = subs[subi];
                    std::string et = errTypes[(si + ec + ei*7) % errTypes.size()];
                    std::string qnum = "第" + std::to_string(10+ec*3+si%5) + "题";
                    std::string kp = sub + "-" + (sub=="语文"?"阅读理解":sub=="数学"?"函数与导数":sub=="英语"?"语法填空":sub=="物理"?"力学分析":sub=="化学"?"化学方程式":sub=="生物"?"遗传与变异":sub=="历史"?"中国古代史":sub=="政治"?"经济生活":"自然地理");
                    addErr(allStuIds[si], eid, sub, qnum+" "+kp, "错误答案A", "正确答案B", et, kp);
                }
            }
        }
        util::writeJSON("errors.json", errors);
    }

    // ========== GOALS ==========
    if (!fileOrDirExists("data/goals.json")) {
        json goals = json::array();
        for (size_t si = 0; si < std::min((size_t)15, allStuIds.size()); si++) {
            json g;
            g["student_id"] = allStuIds[si];
            g["subject"] = allStuLevels[si] >= 3 ? "数学" : "英语";
            g["target_score"] = (allStuLevels[si] >= 3) ? "120" : "140";
            g["exam_name"] = "高一上学期期末考试";
            g["deadline"] = "2026-01-15";
            g["created_at"] = util::getCurrentTime();
            goals.push_back(g);
        }
        util::writeJSON("goals.json", goals);
    }

    // ========== POINTS ==========
    if (!fileOrDirExists("data/points.json")) {
        json points = json::array();
        for (size_t si = 0; si < 10; si++) {
            json p;
            p["student_id"] = allStuIds[si];
            p["points"] = 50 + (int)(si * 15);
            p["history"] = json::array();
            json h;
            h["reason"] = "登录系统"; h["points_added"] = 10; h["timestamp"] = util::getCurrentTime();
            p["history"].push_back(h);
            points.push_back(p);
        }
        util::writeJSON("points.json", points);
    }

    // ========== CHECKINS ==========
    if (!fileOrDirExists("data/checkins.json")) {
        json checkins = json::array();
        for (size_t si = 0; si < 8; si++) {
            for (int d = 1; d <= 5; d++) {
                json c;
                c["student_id"] = allStuIds[si];
                c["mood"] = 3 + (int)((si+d)%3);
                c["note"] = "心情不错";
                c["timestamp"] = "2026-05-" + std::to_string(20+d);
                checkins.push_back(c);
            }
        }
        util::writeJSON("checkins.json", checkins);
    }

    std::cout << "Data seeding complete (50 students, 3 exams, 9 classes, rich data)." << std::endl;
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

    // Serve static files (use absolute path based on exe location)
#ifdef _WIN32
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::string exeDir(exePath);
    exeDir = exeDir.substr(0, exeDir.find_last_of("\\/"));
    std::string webDir = exeDir + "\\web";
#elif defined(__APPLE__)
    char exePath[PATH_MAX];
    uint32_t bufSize = PATH_MAX;
    if (_NSGetExecutablePath(exePath, &bufSize) != 0) {
        std::cerr << "Fatal: failed to get executable path" << std::endl;
        return 1;
    }
    char* resolved = realpath(exePath, NULL);
    std::string exeDir(resolved ? resolved : exePath);
    if (resolved) free(resolved);
    size_t pos = exeDir.find_last_of('/');
    exeDir = (pos != std::string::npos) ? exeDir.substr(0, pos) : exeDir;
    std::string webDir = exeDir + "/web";
#else
    char exePath[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
    if (len == -1) {
        std::cerr << "Fatal: failed to get executable path" << std::endl;
        return 1;
    }
    exePath[len] = '\0';
    std::string exeDir(exePath);
    size_t pos = exeDir.find_last_of('/');
    exeDir = (pos != std::string::npos) ? exeDir.substr(0, pos) : exeDir;
    std::string webDir = exeDir + "/web";
#endif
    svr.set_mount_point("/", webDir.c_str());

    std::cout << "EduGrade Server started on http://localhost:8080" << std::endl;
    svr.listen("0.0.0.0", 8080);

    return 0;
}
