#include "student.h"
#include "util.h"

namespace student {

json getStudents(const std::string& search, const std::string& cls, int page, int size) {
    json students = util::readJSON("students.json");
    json filtered = json::array();
    for (auto& s : students) {
        std::string name = util::toLower(s.value("name", ""));
        std::string c = s.value("class", "");
        std::string sid = s.value("id", "");
        if (!search.empty() && name.find(util::toLower(search)) == std::string::npos && sid.find(search) == std::string::npos)
            continue;
        if (!cls.empty() && c != cls) continue;
        filtered.push_back(s);
    }
    int total = filtered.size();
    int start = (page - 1) * size;
    int end = std::min(start + size, total);
    json result;
    result["total"] = total;
    result["page"] = page;
    result["size"] = size;
    result["data"] = json::array();
    for (int i = start; i < end; i++) result["data"].push_back(filtered[i]);
    return util::jsonResponse(true, "ok", result);
}

json addStudent(const json& data) {
    json students = util::readJSON("students.json");
    std::string sid = data.value("id", "");
    if (sid.empty()) return util::errorResponse("学号不能为空");
    for (auto& s : students)
        if (s["id"] == sid) return util::errorResponse("学号已存在");
    json s;
    s["id"] = sid;
    s["name"] = data.value("name", "");
    s["birthday"] = data.value("birthday", "");
    s["class"] = data.value("class", "");
    s["gender"] = data.value("gender", "");
    s["parent_id"] = data.value("parent_id", "");
    s["enrolled_at"] = util::getCurrentTime();
    students.push_back(s);
    util::writeJSON("students.json", students);
    return util::jsonResponse(true, "学生添加成功", s);
}

json updateStudent(const std::string& id, const json& data) {
    json students = util::readJSON("students.json");
    for (auto& s : students) {
        if (s["id"] == id) {
            if (data.contains("name")) s["name"] = data["name"];
            if (data.contains("birthday")) s["birthday"] = data["birthday"];
            if (data.contains("class")) s["class"] = data["class"];
            if (data.contains("gender")) s["gender"] = data["gender"];
            if (data.contains("parent_id")) s["parent_id"] = data["parent_id"];
            util::writeJSON("students.json", students);
            return util::jsonResponse(true, "学生信息更新成功", s);
        }
    }
    return util::errorResponse("学生不存在");
}

json deleteStudent(const std::string& id) {
    json students = util::readJSON("students.json");
    json filtered = json::array();
    bool found = false;
    for (auto& s : students) {
        if (s["id"] == id) { found = true; continue; }
        filtered.push_back(s);
    }
    if (!found) return util::errorResponse("学生不存在");
    util::writeJSON("students.json", filtered);
    return util::jsonResponse(true, "学生已删除");
}

json getClasses() {
    json classes = util::readJSON("classes.json");
    return util::jsonResponse(true, "ok", classes);
}

json addClass(const json& data) {
    json classes = util::readJSON("classes.json");
    std::string name = data.value("name", "");
    if (name.empty()) return util::errorResponse("班级名称不能为空");
    for (auto& c : classes)
        if (c["name"] == name) return util::errorResponse("班级已存在");
    json cls;
    cls["id"] = "C" + std::to_string(classes.size() + 1);
    cls["name"] = name;
    cls["grade"] = data.value("grade", "");
    cls["created_at"] = util::getCurrentTime();
    classes.push_back(cls);
    util::writeJSON("classes.json", classes);
    return util::jsonResponse(true, "班级添加成功", cls);
}

json updateClass(const std::string& id, const json& data) {
    json classes = util::readJSON("classes.json");
    for (auto& c : classes) {
        if (c["id"] == id) {
            if (data.contains("name")) c["name"] = data["name"];
            if (data.contains("grade")) c["grade"] = data["grade"];
            util::writeJSON("classes.json", classes);
            return util::jsonResponse(true, "班级更新成功", c);
        }
    }
    return util::errorResponse("班级不存在");
}

json deleteClass(const std::string& id) {
    json classes = util::readJSON("classes.json");
    json filtered = json::array();
    for (auto& c : classes) {
        if (c["id"] == id) continue;
        filtered.push_back(c);
    }
    util::writeJSON("classes.json", filtered);
    return util::jsonResponse(true, "班级已删除");
}

json getSubjects() {
    json subjects = util::readJSON("subjects.json");
    return util::jsonResponse(true, "ok", subjects);
}

json addSubject(const json& data) {
    json subjects = util::readJSON("subjects.json");
    std::string name = data.value("name", "");
    if (name.empty()) return util::errorResponse("科目名称不能为空");
    for (auto& s : subjects)
        if (s["name"] == name) return util::errorResponse("科目已存在");
    json sub;
    sub["id"] = "SUB" + std::to_string(subjects.size() + 1);
    sub["name"] = name;
    sub["full_score"] = data.value("full_score", 100);
    subjects.push_back(sub);
    util::writeJSON("subjects.json", subjects);
    return util::jsonResponse(true, "科目添加成功", sub);
}

json getEnrollmentStats() {
    json students = util::readJSON("students.json");
    json classes = util::readJSON("classes.json");
    std::map<std::string, int> classCount;
    for (auto& s : students) {
        std::string cls = s.value("class", "");
        if (!cls.empty()) classCount[cls]++;
    }
    json result = json::array();
    for (auto& c : classes) {
        json item;
        item["class"] = c["name"];
        item["count"] = classCount[c["name"].get<std::string>()];
        result.push_back(item);
    }
    return util::jsonResponse(true, "ok", result);
}

}
