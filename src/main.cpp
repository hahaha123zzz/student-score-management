/*
 * EduGrade 程序入口（第 5 部分）
 *
 * 这个文件现在只负责两件事：
 * 1. 按顺序组合前 4 个实现分段
 * 2. 注册路由并启动 HTTP 服务
 *
 * 之所以采用“main.cpp + 4 个分段 .cpp”的方式，
 * 是为了继续满足“不编写自定义头文件”的要求。
 */

#include "core_part1_base.cpp"
#include "core_part2_storage.cpp"
#include "core_part3_logic.cpp"
#include "core_part4_handlers.cpp"

int main() {
    std::srand(static_cast<unsigned int>(std::time(NULL)));
    ensureSeedData();

    Server server;

    /*
     * 统一处理浏览器的预检请求。
     * 这样前端页面访问所有接口时，都能复用同一套跨域配置。
     */
    server.Options(".*", [](const Request&, Response& res) {
        addCors(res);
        res.status = 200;
    });

    /* ---------- 登录与认证 ---------- */
    server.Post("/api/login", handleLogin);
    server.Get("/api/auth/check", handleAuthCheck);
    server.Post("/api/change-password", handleChangePassword);

    /* ---------- 班级与概览 ---------- */
    server.Get("/api/classes", handleListClasses);
    server.Get("/api/stats", handleExamStats);
    server.Get("/api/logs", handleLogs);

    /* ---------- 学生管理 ---------- */
    server.Get("/api/students", handleListStudents);
    server.Post("/api/students", handleAddStudent);
    server.Get(R"(/api/students/(.+))", [](const Request& req, Response& res) {
        handleGetStudent(req, res, req.matches[1]);
    });
    server.Put(R"(/api/students/(.+))", [](const Request& req, Response& res) {
        handleUpdateStudent(req, res, req.matches[1]);
    });
    server.Delete(R"(/api/students/(.+))", [](const Request& req, Response& res) {
        handleDeleteStudent(req, res, req.matches[1]);
    });

    /* ---------- 科目管理 ---------- */
    server.Get("/api/subjects", handleListSubjects);
    server.Post("/api/subjects", handleAddSubject);
    server.Put(R"(/api/subjects/(.+))", [](const Request& req, Response& res) {
        handleUpdateSubject(req, res, req.matches[1]);
    });
    server.Delete(R"(/api/subjects/(.+))", [](const Request& req, Response& res) {
        handleDeleteSubject(req, res, req.matches[1]);
    });

    /* ---------- 考试管理 ---------- */
    server.Get("/api/exams", handleListExams);
    server.Post("/api/exams", handleAddExam);
    server.Put(R"(/api/exams/(.+))", [](const Request& req, Response& res) {
        handleUpdateExam(req, res, req.matches[1]);
    });
    server.Delete(R"(/api/exams/(.+))", [](const Request& req, Response& res) {
        handleDeleteExam(req, res, req.matches[1]);
    });

    /* ---------- 成绩管理 ---------- */
    server.Get("/api/grades", handleListGrades);
    server.Post("/api/grades", handleUpsertGrade);
    server.Delete(R"(/api/grades/([^/]+)/([^/]+))", [](const Request& req, Response& res) {
        handleDeleteGrade(req, res, req.matches[1], req.matches[2]);
    });
    server.Get(R"(/api/grades/student/(.+))", [](const Request& req, Response& res) {
        handleStudentGrades(req, res, req.matches[1]);
    });

    /* ---------- 排名与导出 ---------- */
    server.Get("/api/rank/grade", handleGradeRank);
    server.Get("/api/rank/class", handleClassRank);
    server.Get("/api/export", handleExportCsv);

    /* ---------- 静态页面 ---------- */
    server.Get("/", [](const Request&, Response& res) {
        res.set_redirect("/index.html");
    });
    server.set_mount_point("/", "./web");

    std::cout << "EduGrade 服务已启动：http://localhost:8080" << std::endl;
    server.listen("0.0.0.0", 8080);
    return 0;
}
