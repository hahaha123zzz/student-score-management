#include "grade.h"
#include "util.h"
#include <sstream>

namespace grade {

json setGrades(const json& data) {
    std::string examId = data.value("exam_id", "");
    std::string studentId = data.value("student_id", "");
    if (examId.empty() || studentId.empty())
        return util::errorResponse("考试ID和学生ID不能为空");

    json exams = util::readJSON("exams.json");
    json examData;
    bool foundExam = false;
    for (auto& e : exams) {
        if (e["id"] == examId) { examData = e; foundExam = true; break; }
    }
    if (!foundExam) return util::errorResponse("考试不存在");
    if (examData["status"] == "locked") return util::errorResponse("成绩已锁定");

    json grades = util::readJSON("grades.json");
    int idx = -1;
    for (int i = 0; i < (int)grades.size(); i++) {
        if (grades[i]["student_id"] == studentId && grades[i]["exam_id"] == examId) {
            idx = i; break;
        }
    }

    json entry;
    entry["student_id"] = studentId;
    entry["exam_id"] = examId;
    entry["scores"] = data.value("scores", json::object());
    entry["comments"] = data.value("comments", json::object());
    entry["is_makeup"] = data.value("is_makeup", false);
    entry["submitted_by"] = data.value("submitted_by", "system");
    entry["submitted_at"] = util::getCurrentTime();

    if (idx >= 0) grades[idx] = entry;
    else grades.push_back(entry);

    util::writeJSON("grades.json", grades);
    return util::jsonResponse(true, "成绩保存成功", entry);
}

json importCSV(const std::string& csvContent, const std::string& examId) {
    std::istringstream stream(csvContent);
    std::string line;
    getline(stream, line);
    std::string header = line;

    std::vector<std::string> subjects;
    std::istringstream hss(header);
    std::string col;
    getline(hss, col, ','); // skip 学号
    getline(hss, col, ','); // skip 姓名
    while (getline(hss, col, ',')) subjects.push_back(col);

    json result = json::array();
    int imported = 0;
    while (getline(stream, line)) {
        if (line.empty()) continue;
        std::istringstream lss(line);
        std::string sid, sname;
        getline(lss, sid, ',');
        getline(lss, sname, ',');

        json scores = json::object();
        for (size_t i = 0; i < subjects.size(); i++) {
            std::string val;
            if (getline(lss, val, ','))
                scores[subjects[i]] = val;
        }

        json data;
        data["exam_id"] = examId;
        data["student_id"] = sid;
        data["scores"] = scores;
        data["submitted_by"] = "import";
        auto resp = setGrades(data);
        if (resp["success"]) imported++;
        result.push_back(resp);
    }
    return util::jsonResponse(true, "导入完成: " + std::to_string(imported) + " 条", result);
}

json exportCSV(const std::string& examId) {
    json exams = util::readJSON("exams.json");
    json examData;
    bool found = false;
    for (auto& e : exams) {
        if (e["id"] == examId) { examData = e; found = true; break; }
    }
    if (!found) return util::errorResponse("考试不存在");

    json grades = util::readJSON("grades.json");
    json students = util::readJSON("students.json");

    std::vector<std::string> subjects;
    for (auto& s : examData["subjects"]) subjects.push_back(s.get<std::string>());

    std::string csv = "学号,姓名";
    for (auto& s : subjects) csv += "," + s;
    csv += "\r\n";

    for (auto& g : grades) {
        if (g["exam_id"] != examId) continue;
        std::string sid = g["student_id"];
        csv += sid + ",";
        std::string sname = sid;
        for (auto& s : students)
            if (s["id"] == sid) { sname = s.value("name", sid); break; }
        csv += sname;
        for (auto& s : subjects) {
            csv += ",";
            if (g["scores"].contains(s))
                csv += g["scores"][s].get<std::string>();
        }
        csv += "\r\n";
    }

    json resp;
    resp["csv"] = csv;
    return util::jsonResponse(true, "ok", resp);
}

json lockGrades(const std::string& examId) {
    json exams = util::readJSON("exams.json");
    for (auto& e : exams) {
        if (e["id"] == examId) {
            e["status"] = "locked";
            util::writeJSON("exams.json", exams);
            return util::jsonResponse(true, "成绩已锁定");
        }
    }
    return util::errorResponse("考试不存在");
}

json getGrades(const std::string& examId, const std::string& studentId) {
    json grades = util::readJSON("grades.json");
    json students = util::readJSON("students.json");
    json filtered = json::array();
    for (auto& g : grades) {
        if (!examId.empty() && g["exam_id"] != examId) continue;
        if (!studentId.empty() && g["student_id"] != studentId) continue;
        for (auto& s : students) {
            if (s["id"] == g["student_id"]) {
                g["student_name"] = s.value("name", "");
                g["class"] = s.value("class", "");
                break;
            }
        }
        filtered.push_back(g);
    }
    return util::jsonResponse(true, "ok", filtered);
}

json getStudentGrades(const std::string& studentId) {
    return getGrades("", studentId);
}

json markMakeup(const std::string& studentId, const std::string& examId, const json& scores) {
    json data;
    data["exam_id"] = examId;
    data["student_id"] = studentId;
    data["scores"] = scores;
    data["is_makeup"] = true;
    return setGrades(data);
}

}
