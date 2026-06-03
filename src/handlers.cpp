#include "handlers.h"
#include "models.h"
#include "storage.h"
#include "utils.h"

namespace handlers {

    std::string parseBodyField(const std::string& body, const std::string& key) {
        std::string searchKey = "\"" + key + "\":\"";
        size_t pos = body.find(searchKey);
        if (pos == std::string::npos) {
            searchKey = "\"" + key + "\":";
            pos = body.find(searchKey);
            if (pos == std::string::npos) return "";
            pos += searchKey.size();
        } else {
            pos += searchKey.size();
            size_t end = body.find("\"", pos);
            if (end == std::string::npos) return "";
            return body.substr(pos, end - pos);
        }
        size_t end = body.find_first_of(",}", pos);
        if (end == std::string::npos) return "";
        return utils::trim(body.substr(pos, end - pos));
    }

    // ===== 权限检查 =====
    bool isAdmin(const std::string& token) {
        if (!utils::isTokenValid(token)) return false;
        std::string role = utils::getUserRoleByToken(token);
        return role == "admin";
    }

    // ===== 登录 =====
    std::string login(const std::string& body) {
        std::string username = parseBodyField(body, "username");
        std::string password = parseBodyField(body, "password");

        std::vector<std::vector<std::string>> users = storage::getUsers();
        for (size_t i = 0; i < users.size(); i++) {
            Person p = Person::fromCSV(users[i]);
            if (p.getId() == username || p.getName() == username) {
                if (p.verifyPassword(password)) {
                    std::string token = utils::generateToken();
                    utils::storeToken(token, p.getId());
                    utils::setUserRole(p.getId(), p.getRole());
                    std::string data = "{";
                    data += "\"token\":\"" + utils::jsonEscape(token) + "\",";
                    data += "\"role\":\"" + utils::jsonEscape(p.getRole()) + "\",";
                    data += "\"name\":\"" + utils::jsonEscape(p.getName()) + "\",";
                    data += "\"user_id\":\"" + utils::jsonEscape(username) + "\"";
                    data += "}";
                    utils::logAction(username, "登录", "用户登录系统");
                    return utils::jsonResponse(true, "登录成功", data);
                }
                return utils::errorResponse("密码错误");
            }
        }
        return utils::errorResponse("用户不存在");
    }

    std::string changePassword(const std::string& token, const std::string& body) {
        if (!utils::isTokenValid(token)) return utils::errorResponse("请先登录");
        std::string uid = utils::getUserIdByToken(token);
        std::string oldPwd = parseBodyField(body, "old_password");
        std::string newPwd = parseBodyField(body, "new_password");

        if (newPwd.size() < 4) return utils::errorResponse("新密码至少4位");

        std::vector<std::vector<std::string>> users = storage::getUsers();
        for (size_t i = 0; i < users.size(); i++) {
            Person p = Person::fromCSV(users[i]);
            if (p.getId() == uid) {
                if (!p.verifyPassword(oldPwd))
                    return utils::errorResponse("原密码错误");
                p.setPassword(utils::hashPassword(newPwd));
                users[i] = p.toCSV();
                storage::saveUsers(users);
                return utils::jsonResponse(true, "密码修改成功", "{}");
            }
        }
        return utils::errorResponse("用户不存在");
    }

    std::string checkAuth(const std::string& token) {
        if (!utils::isTokenValid(token)) return utils::errorResponse("未登录或登录已过期");
        std::string uid = utils::getUserIdByToken(token);
        std::string role = utils::getUserRoleByToken(token);
        std::string data = "{\"user_id\":\"" + utils::jsonEscape(uid) + "\",\"role\":\"" + utils::jsonEscape(role) + "\"}";
        return utils::jsonResponse(true, "已登录", data);
    }

    // ===== 用户管理 =====
    std::string getUsers() {
        std::vector<std::vector<std::string>> users = storage::getUsers();
        std::string data = "[";
        for (size_t i = 0; i < users.size(); i++) {
            if (i > 0) data += ",";
            Person p = Person::fromCSV(users[i]);
            data += "{";
            data += "\"id\":\"" + utils::jsonEscape(p.getId()) + "\",";
            data += "\"name\":\"" + utils::jsonEscape(p.getName()) + "\",";
            data += "\"role\":\"" + utils::jsonEscape(p.getRole()) + "\",";
            data += "\"created_at\":\"" + utils::jsonEscape(p.getCreatedAt()) + "\"";
            data += "}";
        }
        data += "]";
        return utils::jsonResponse(true, "ok", data);
    }

    std::string addUser(const std::string& body) {
        std::string userId = parseBodyField(body, "id");
        std::string name = parseBodyField(body, "name");
        std::string role = parseBodyField(body, "role");
        if (userId.empty()) return utils::errorResponse("用户ID不能为空");
        if (role.empty()) role = "student";

        std::vector<std::vector<std::string>> users = storage::getUsers();
        for (size_t i = 0; i < users.size(); i++) {
            if (users[i][0] == userId) return utils::errorResponse("用户ID已存在");
        }

        Person p(userId, name, utils::hashPassword("123456"), role, utils::getCurrentTime());
        users.push_back(p.toCSV());
        storage::saveUsers(users);
        return utils::jsonResponse(true, "用户创建成功", "{}");
    }

    std::string updateUser(const std::string& userId, const std::string& body) {
        std::vector<std::vector<std::string>> users = storage::getUsers();
        for (size_t i = 0; i < users.size(); i++) {
            if (users[i][0] == userId) {
                std::string name = parseBodyField(body, "name");
                std::string role = parseBodyField(body, "role");
                if (!name.empty()) users[i][1] = name;
                if (!role.empty()) users[i][3] = role;
                storage::saveUsers(users);
                return utils::jsonResponse(true, "用户信息更新成功", "{}");
            }
        }
        return utils::errorResponse("用户不存在");
    }

    std::string deleteUser(const std::string& userId) {
        std::vector<std::vector<std::string>> users = storage::getUsers();
        std::vector<std::vector<std::string>> filtered;
        for (size_t i = 0; i < users.size(); i++) {
            if (users[i][0] != userId) filtered.push_back(users[i]);
        }
        if (filtered.size() == users.size()) return utils::errorResponse("用户不存在");
        storage::saveUsers(filtered);
        return utils::jsonResponse(true, "用户已删除", "{}");
    }

    // ===== 学生管理 =====
    std::string getStudents(const std::string& queryString) {
        std::string search = utils::getQueryParam(queryString, "search");
        std::string cls = utils::getQueryParam(queryString, "class");
        std::string pageStr = utils::getQueryParam(queryString, "page");
        std::string sizeStr = utils::getQueryParam(queryString, "size");
        int page = pageStr.empty() ? 1 : std::stoi(pageStr);
        int size = sizeStr.empty() ? 20 : std::stoi(sizeStr);

        std::vector<std::vector<std::string>> all = storage::getStudents();
        std::vector<Student> filtered;
        for (size_t i = 0; i < all.size(); i++) {
            Student s = Student::fromCSV(all[i]);
            if (!search.empty()) {
                std::string lowName = utils::toLower(s.getName());
                std::string lowSearch = utils::toLower(search);
                std::string sid = s.getId();
                if (lowName.find(lowSearch) == std::string::npos && sid.find(search) == std::string::npos)
                    continue;
            }
            if (!cls.empty() && s.getClassName() != cls) continue;
            filtered.push_back(s);
        }

        int total = (int)filtered.size();
        int start = (page - 1) * size;
        int end = start + size;
        if (end > total) end = total;

        std::string data = "{";
        data += "\"total\":" + std::to_string(total) + ",";
        data += "\"page\":" + std::to_string(page) + ",";
        data += "\"size\":" + std::to_string(size) + ",";
        data += "\"data\":[";
        for (int i = start; i < end; i++) {
            if (i > start) data += ",";
            data += "{";
            data += "\"id\":\"" + utils::jsonEscape(filtered[i].getId()) + "\",";
            data += "\"name\":\"" + utils::jsonEscape(filtered[i].getName()) + "\",";
            data += "\"birthday\":\"" + utils::jsonEscape(filtered[i].getBirthday()) + "\",";
            data += "\"class\":\"" + utils::jsonEscape(filtered[i].getClassName()) + "\",";
            data += "\"gender\":\"" + utils::jsonEscape(filtered[i].getGender()) + "\"";
            data += "}";
        }
        data += "]";
        data += "}";
        return utils::jsonResponse(true, "ok", data);
    }

    std::string addStudent(const std::string& body) {
        std::string sid = parseBodyField(body, "id");
        std::string name = parseBodyField(body, "name");
        std::string className = parseBodyField(body, "class");
        std::string gender = parseBodyField(body, "gender");
        std::string birthday = parseBodyField(body, "birthday");

        if (sid.empty()) return utils::errorResponse("学号不能为空");

        std::vector<std::vector<std::string>> all = storage::getStudents();
        for (size_t i = 0; i < all.size(); i++) {
            if (all[i][0] == sid) return utils::errorResponse("学号已存在");
        }
        std::vector<std::string> row;
        row.push_back(sid);
        row.push_back(name);
        row.push_back(birthday);
        row.push_back(className);
        row.push_back(gender);
        row.push_back(utils::getCurrentTime());
        all.push_back(row);
        storage::saveStudents(all);
        return utils::jsonResponse(true, "学生添加成功", "{}");
    }

    std::string updateStudent(const std::string& id, const std::string& body) {
        std::vector<std::vector<std::string>> all = storage::getStudents();
        for (size_t i = 0; i < all.size(); i++) {
            if (all[i][0] == id) {
                std::string name = parseBodyField(body, "name");
                std::string birthday = parseBodyField(body, "birthday");
                std::string className = parseBodyField(body, "class");
                std::string gender = parseBodyField(body, "gender");
                if (!name.empty()) all[i][1] = name;
                if (!birthday.empty()) all[i][2] = birthday;
                if (!className.empty()) all[i][3] = className;
                if (!gender.empty()) all[i][4] = gender;
                storage::saveStudents(all);
                return utils::jsonResponse(true, "学生信息更新成功", "{}");
            }
        }
        return utils::errorResponse("学生不存在");
    }

    std::string deleteStudent(const std::string& id) {
        std::vector<std::vector<std::string>> all = storage::getStudents();
        std::vector<std::vector<std::string>> filtered;
        for (size_t i = 0; i < all.size(); i++) {
            if (all[i][0] != id) filtered.push_back(all[i]);
        }
        if (filtered.size() == all.size()) return utils::errorResponse("学生不存在");
        storage::saveStudents(filtered);
        return utils::jsonResponse(true, "学生已删除", "{}");
    }

    // ===== 班级管理 =====
    std::string getClasses() {
        std::vector<std::vector<std::string>> all = storage::getClasses();
        std::string data = "[";
        for (size_t i = 0; i < all.size(); i++) {
            if (i > 0) data += ",";
            Cls c = Cls::fromCSV(all[i]);
            data += "{";
            data += "\"id\":\"" + utils::jsonEscape(c.getId()) + "\",";
            data += "\"name\":\"" + utils::jsonEscape(c.getName()) + "\",";
            data += "\"grade\":\"" + utils::jsonEscape(c.getGrade()) + "\"";
            data += "}";
        }
        data += "]";
        return utils::jsonResponse(true, "ok", data);
    }

    std::string addClass(const std::string& body) {
        std::string name = parseBodyField(body, "name");
        std::string grade = parseBodyField(body, "grade");
        if (name.empty()) return utils::errorResponse("班级名称不能为空");

        std::vector<std::vector<std::string>> all = storage::getClasses();
        for (size_t i = 0; i < all.size(); i++) {
            if (all[i][1] == name) return utils::errorResponse("班级已存在");
        }

        std::string newId = "CLS" + std::to_string(all.size() + 1);
        std::vector<std::string> row;
        row.push_back(newId);
        row.push_back(name);
        row.push_back(grade);
        row.push_back(utils::getCurrentTime());
        all.push_back(row);
        storage::saveClasses(all);
        return utils::jsonResponse(true, "班级添加成功", "{}");
    }

    std::string updateClass(const std::string& id, const std::string& body) {
        std::vector<std::vector<std::string>> all = storage::getClasses();
        for (size_t i = 0; i < all.size(); i++) {
            if (all[i][0] == id) {
                std::string name = parseBodyField(body, "name");
                std::string grade = parseBodyField(body, "grade");
                if (!name.empty()) all[i][1] = name;
                if (!grade.empty()) all[i][2] = grade;
                storage::saveClasses(all);
                return utils::jsonResponse(true, "班级更新成功", "{}");
            }
        }
        return utils::errorResponse("班级不存在");
    }

    std::string deleteClass(const std::string& id) {
        std::vector<std::vector<std::string>> all = storage::getClasses();
        std::vector<std::vector<std::string>> filtered;
        for (size_t i = 0; i < all.size(); i++) {
            if (all[i][0] != id) filtered.push_back(all[i]);
        }
        if (filtered.size() == all.size()) return utils::errorResponse("班级不存在");
        storage::saveClasses(filtered);
        return utils::jsonResponse(true, "班级已删除", "{}");
    }

    // ===== 科目管理 =====
    std::string getSubjects() {
        std::vector<std::vector<std::string>> all = storage::getSubjects();
        std::string data = "[";
        for (size_t i = 0; i < all.size(); i++) {
            if (i > 0) data += ",";
            Subject s = Subject::fromCSV(all[i]);
            data += "{";
            data += "\"id\":\"" + utils::jsonEscape(s.getId()) + "\",";
            data += "\"name\":\"" + utils::jsonEscape(s.getName()) + "\",";
            data += "\"full_score\":" + std::to_string(s.getFullScore());
            data += "}";
        }
        data += "]";
        return utils::jsonResponse(true, "ok", data);
    }

    std::string addSubject(const std::string& body) {
        std::string name = parseBodyField(body, "name");
        std::string fullScoreStr = parseBodyField(body, "full_score");
        int fullScore = fullScoreStr.empty() ? 100 : std::stoi(fullScoreStr);
        if (name.empty()) return utils::errorResponse("科目名称不能为空");

        std::vector<std::vector<std::string>> all = storage::getSubjects();
        for (size_t i = 0; i < all.size(); i++) {
            if (all[i][1] == name) return utils::errorResponse("科目已存在");
        }

        std::string newId = "SUB" + std::to_string(all.size() + 1);
        std::vector<std::string> row;
        row.push_back(newId);
        row.push_back(name);
        row.push_back(std::to_string(fullScore));
        all.push_back(row);
        storage::saveSubjects(all);
        return utils::jsonResponse(true, "科目添加成功", "{}");
    }

    // ===== 考试管理 =====
    std::string getExams(const std::string& queryString) {
        std::string status = utils::getQueryParam(queryString, "status");
        std::vector<std::vector<std::string>> all = storage::getExams();
        std::string data = "[";
        bool first = true;
        for (size_t i = 0; i < all.size(); i++) {
            Exam e = Exam::fromCSV(all[i]);
            if (!status.empty() && e.getStatus() != status) continue;
            if (!first) data += ","; first = false;
            data += "{";
            data += "\"id\":\"" + utils::jsonEscape(e.getId()) + "\",";
            data += "\"name\":\"" + utils::jsonEscape(e.getName()) + "\",";
            data += "\"date\":\"" + utils::jsonEscape(e.getDate()) + "\",";
            data += "\"status\":\"" + utils::jsonEscape(e.getStatus()) + "\",";
            data += "\"subjects\":[";
            std::vector<std::string> slist = e.getSubjectList();
            for (size_t j = 0; j < slist.size(); j++) {
                if (j > 0) data += ",";
                data += "\"" + utils::jsonEscape(slist[j]) + "\"";
            }
            data += "],";
            data += "\"weight_config\":{";
            std::vector<std::string> configs = utils::split(e.getWeightConfig(), '|');
            for (size_t j = 0; j < configs.size(); j++) {
                std::vector<std::string> parts = utils::split(configs[j], ':');
                if (parts.size() >= 3) {
                    if (j > 0) data += ",";
                    data += "\"" + utils::jsonEscape(parts[0]) + "\":{";
                    data += "\"full\":" + parts[1] + ",";
                    data += "\"weight\":" + parts[2];
                    data += "}";
                }
            }
            data += "}";
            data += "}";
        }
        data += "]";
        return utils::jsonResponse(true, "ok", data);
    }

    std::string addExam(const std::string& body) {
        std::string name = parseBodyField(body, "name");
        std::string date = parseBodyField(body, "date");
        if (name.empty()) return utils::errorResponse("考试名称不能为空");

        std::vector<std::vector<std::string>> all = storage::getExams();
        std::string newId = "EXAM" + std::to_string(all.size() + 1);

        std::string subjectsStr = "语文|数学|英语";
        std::string weightConfig = "语文:150:1.0|数学:150:1.0|英语:150:1.0";

        std::vector<std::string> row;
        row.push_back(newId);
        row.push_back(name);
        row.push_back(date);
        row.push_back("draft");
        row.push_back(subjectsStr);
        row.push_back(weightConfig);
        all.push_back(row);
        storage::saveExams(all);
        return utils::jsonResponse(true, "考试创建成功", "{}");
    }

    std::string updateExam(const std::string& id, const std::string& body) {
        std::vector<std::vector<std::string>> all = storage::getExams();
        for (size_t i = 0; i < all.size(); i++) {
            if (all[i][0] == id) {
                std::string name = parseBodyField(body, "name");
                std::string date = parseBodyField(body, "date");
                if (!name.empty()) all[i][1] = name;
                if (!date.empty()) all[i][2] = date;
                storage::saveExams(all);
                return utils::jsonResponse(true, "考试更新成功", "{}");
            }
        }
        return utils::errorResponse("考试不存在");
    }

    std::string deleteExam(const std::string& id) {
        std::vector<std::vector<std::string>> all = storage::getExams();
        std::vector<std::vector<std::string>> filtered;
        for (size_t i = 0; i < all.size(); i++) {
            if (all[i][0] != id) filtered.push_back(all[i]);
        }
        if (filtered.size() == all.size()) return utils::errorResponse("考试不存在");
        storage::saveExams(filtered);
        return utils::jsonResponse(true, "考试已删除", "{}");
    }

    std::string publishExam(const std::string& id) {
        std::vector<std::vector<std::string>> all = storage::getExams();
        for (size_t i = 0; i < all.size(); i++) {
            if (all[i][0] == id) {
                if (all[i][3] == "locked") return utils::errorResponse("已锁定不能发布");
                all[i][3] = "published";
                storage::saveExams(all);
                return utils::jsonResponse(true, "成绩已发布", "{}");
            }
        }
        return utils::errorResponse("考试不存在");
    }

    std::string retractExam(const std::string& id) {
        std::vector<std::vector<std::string>> all = storage::getExams();
        for (size_t i = 0; i < all.size(); i++) {
            if (all[i][0] == id) {
                if (all[i][3] == "locked") return utils::errorResponse("已锁定不能撤回");
                all[i][3] = "draft";
                storage::saveExams(all);
                return utils::jsonResponse(true, "成绩已撤回", "{}");
            }
        }
        return utils::errorResponse("考试不存在");
    }

    // ===== 成绩管理 =====
    std::string setGrades(const std::string& body) {
        std::string examId = parseBodyField(body, "exam_id");
        std::string studentId = parseBodyField(body, "student_id");
        std::string scores = parseBodyField(body, "scores");
        if (examId.empty() || studentId.empty())
            return utils::errorResponse("考试ID和学生ID不能为空");

        std::vector<std::vector<std::string>> grades = storage::getGrades();
        bool found = false;
        for (size_t i = 0; i < grades.size(); i++) {
            if (grades[i][1] == studentId && grades[i][2] == examId) {
                grades[i][3] = scores;
                found = true;
                break;
            }
        }
        if (!found) {
            std::vector<std::string> row;
            row.push_back(std::to_string(grades.size() + 1));
            row.push_back(studentId);
            row.push_back(examId);
            row.push_back(scores);
            row.push_back("false");
            row.push_back("admin");
            row.push_back(utils::getCurrentTime());
            grades.push_back(row);
        }
        storage::saveGrades(grades);
        return utils::jsonResponse(true, "成绩保存成功", "{}");
    }

    std::string importCSV(const std::string& body) {
        return utils::errorResponse("CSV导入功能开发中");
    }

    std::string exportCSV(const std::string& queryString) {
        std::string examId = utils::getQueryParam(queryString, "exam_id");
        if (examId.empty()) return utils::errorResponse("请指定考试ID");

        std::vector<std::vector<std::string>> exams = storage::getExams();
        std::string subjects;
        for (size_t i = 0; i < exams.size(); i++) {
            if (exams[i][0] == examId) { subjects = exams[i][4]; break; }
        }
        if (subjects.empty()) return utils::errorResponse("考试不存在");

        std::vector<std::vector<std::string>> grades = storage::getGrades();
        std::vector<std::vector<std::string>> students = storage::getStudents();

        std::vector<std::string> subList = utils::split(subjects, '|');
        std::string csv = "学号,姓名";
        for (size_t i = 0; i < subList.size(); i++) csv += "," + subList[i];
        csv += "\r\n";

        for (size_t i = 0; i < grades.size(); i++) {
            if (grades[i][2] != examId) continue;
            std::string sid = grades[i][1];
            csv += sid + ",";
            std::string sname = sid;
            for (size_t j = 0; j < students.size(); j++) {
                if (students[j][0] == sid) { sname = students[j][1]; break; }
            }
            csv += sname;

            Grade g = Grade::fromCSV(grades[i]);
            std::vector<double> scoreList = g.getScoreList();
            for (size_t j = 0; j < subList.size(); j++) {
                csv += ",";
                if (j < scoreList.size())
                    csv += std::to_string((int)scoreList[j]);
            }
            csv += "\r\n";
        }

        std::string data = "{\"csv\":\"" + utils::jsonEscape(csv) + "\"}";
        return utils::jsonResponse(true, "ok", data);
    }

    std::string lockGrades(const std::string& examId) {
        std::vector<std::vector<std::string>> all = storage::getExams();
        for (size_t i = 0; i < all.size(); i++) {
            if (all[i][0] == examId) {
                all[i][3] = "locked";
                storage::saveExams(all);
                return utils::jsonResponse(true, "成绩已锁定", "{}");
            }
        }
        return utils::errorResponse("考试不存在");
    }

    std::string getGrades(const std::string& queryString) {
        std::string examId = utils::getQueryParam(queryString, "exam_id");
        std::string studentId = utils::getQueryParam(queryString, "student_id");

        std::vector<std::vector<std::string>> grades = storage::getGrades();
        std::vector<std::vector<std::string>> students = storage::getStudents();

        std::string data = "[";
        bool first = true;
        for (size_t i = 0; i < grades.size(); i++) {
            if (!examId.empty() && grades[i][2] != examId) continue;
            if (!studentId.empty() && grades[i][1] != studentId) continue;
            if (!first) data += ","; first = false;

            std::string sname, sclass;
            for (size_t j = 0; j < students.size(); j++) {
                if (students[j][0] == grades[i][1]) {
                    sname = students[j][1];
                    sclass = students[j][3];
                    break;
                }
            }

            data += "{";
            data += "\"student_id\":\"" + utils::jsonEscape(grades[i][1]) + "\",";
            data += "\"exam_id\":\"" + utils::jsonEscape(grades[i][2]) + "\",";
            data += "\"student_name\":\"" + utils::jsonEscape(sname) + "\",";
            data += "\"class\":\"" + utils::jsonEscape(sclass) + "\",";
            data += "\"scores\":\"" + utils::jsonEscape(grades[i][3]) + "\",";
            data += "\"is_makeup\":" + std::string(grades[i][4] == "true" ? "true" : "false");
            data += "}";
        }
        data += "]";
        return utils::jsonResponse(true, "ok", data);
    }

    std::string getStudentGrades(const std::string& studentId) {
        return getGrades("student_id=" + studentId);
    }

    std::string markMakeup(const std::string& body) {
        std::string studentId = parseBodyField(body, "student_id");
        std::string examId = parseBodyField(body, "exam_id");
        std::string scores = parseBodyField(body, "scores");

        std::vector<std::vector<std::string>> grades = storage::getGrades();
        bool found = false;
        for (size_t i = 0; i < grades.size(); i++) {
            if (grades[i][1] == studentId && grades[i][2] == examId) {
                grades[i][3] = scores;
                grades[i][4] = "true";
                found = true;
                break;
            }
        }
        if (!found) return utils::errorResponse("未找到该学生的成绩记录");
        storage::saveGrades(grades);
        return utils::jsonResponse(true, "补考成绩已录入", "{}");
    }

    // ===== 日志 =====
    std::string getLogs(const std::string& queryString) {
        std::string pageStr = utils::getQueryParam(queryString, "page");
        int page = pageStr.empty() ? 1 : std::stoi(pageStr);
        int size = 50;
        return utils::getLogs(page, size, "");
    }

}
