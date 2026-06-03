#include "handlers.h"
#include "models.h"
#include "storage.h"
#include "utils.h"
#include <algorithm>

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
