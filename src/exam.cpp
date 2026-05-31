#include "exam.h"
#include "util.h"

namespace exam {

json getExams(const std::string& status) {
    json exams = util::readJSON("exams.json");
    if (status.empty())
        return util::jsonResponse(true, "ok", exams);
    json filtered = json::array();
    for (auto& e : exams)
        if (e.value("status", "") == status)
            filtered.push_back(e);
    return util::jsonResponse(true, "ok", filtered);
}

json addExam(const json& data) {
    json exams = util::readJSON("exams.json");
    std::string name = data.value("name", "");
    if (name.empty()) return util::errorResponse("考试名称不能为空");

    json e;
    e["id"] = "EXAM" + std::to_string(exams.size() + 1);
    e["name"] = name;
    e["date"] = data.value("date", "");
    e["subjects"] = data.value("subjects", json::array());
    e["status"] = "draft";
    e["weight_config"] = data.value("weight_config", json::object());
    e["created_at"] = util::getCurrentTime();
    exams.push_back(e);
    util::writeJSON("exams.json", exams);
    return util::jsonResponse(true, "考试创建成功", e);
}

json updateExam(const std::string& id, const json& data) {
    json exams = util::readJSON("exams.json");
    for (auto& e : exams) {
        if (e["id"] == id) {
            if (data.contains("name")) e["name"] = data["name"];
            if (data.contains("date")) e["date"] = data["date"];
            if (data.contains("subjects")) e["subjects"] = data["subjects"];
            if (data.contains("weight_config")) e["weight_config"] = data["weight_config"];
            util::writeJSON("exams.json", exams);
            return util::jsonResponse(true, "考试更新成功", e);
        }
    }
    return util::errorResponse("考试不存在");
}

json deleteExam(const std::string& id) {
    json exams = util::readJSON("exams.json");
    json filtered = json::array();
    bool found = false;
    for (auto& e : exams) {
        if (e["id"] == id) { found = true; continue; }
        filtered.push_back(e);
    }
    if (!found) return util::errorResponse("考试不存在");
    util::writeJSON("exams.json", filtered);
    return util::jsonResponse(true, "考试已删除");
}

json publishExam(const std::string& id) {
    json exams = util::readJSON("exams.json");
    for (auto& e : exams) {
        if (e["id"] == id) {
            if (e["status"] == "locked") return util::errorResponse("已锁定不能发布");
            e["status"] = "published";
            util::writeJSON("exams.json", exams);
            return util::jsonResponse(true, "成绩已发布");
        }
    }
    return util::errorResponse("考试不存在");
}

json retractExam(const std::string& id) {
    json exams = util::readJSON("exams.json");
    for (auto& e : exams) {
        if (e["id"] == id) {
            if (e["status"] == "locked") return util::errorResponse("已锁定不能撤回");
            e["status"] = "draft";
            util::writeJSON("exams.json", exams);
            return util::jsonResponse(true, "成绩已撤回");
        }
    }
    return util::errorResponse("考试不存在");
}

}
