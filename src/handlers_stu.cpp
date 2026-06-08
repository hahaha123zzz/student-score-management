#include "handlers.h"
#include "models.h"
#include "storage.h"
#include "utils.h"

namespace handlers {

    // ===== 学生管理 =====

    // ===================================================================
    // 【获取学生列表】处理 GET /api/students?search=&class=&page=1&size=20
    // 支持三种高级功能：
    //   search —— 模糊搜索，按"姓名"或"学号"匹配（大小写不敏感）
    //   class  —— 按班级名称精确筛选
    //   page/size —— 分页，page 从 1 开始，每页 size 条
    //
    // 返回：{total, page, size, data: [{id, name, birthday, class, gender}, ...]}
    //
    // 分页算法：
    //   start = (page - 1) * size     // 起始索引（0-based）
    //   end = start + size            // 结束索引（不包含）
    //   只输出 data[start] ~ data[end-1] 段
    // ===================================================================
    std::string getStudents(const std::string& queryString) {
        std::string search = utils::getQueryParam(queryString, "search");
        std::string cls = utils::getQueryParam(queryString, "class");
        std::string pageStr = utils::getQueryParam(queryString, "page");
        std::string sizeStr = utils::getQueryParam(queryString, "size");
        int page = 1;
        try { if (!pageStr.empty()) page = std::stoi(pageStr); } catch (...) {}
        int size = 20;
        try { if (!sizeStr.empty()) size = std::stoi(sizeStr); } catch (...) {}

        std::vector<std::vector<std::string>> all = storage::getStudents();
        std::vector<Student> filtered;                           // 用于存放筛选后的结果
        for (size_t i = 0; i < all.size(); i++) {
            Student s = Student::fromCSV(all[i]);
            // 模糊搜索：姓名或学号包含关键词
            if (!search.empty()) {
                std::string lowName = utils::toLower(s.getName());
                std::string lowSearch = utils::toLower(search);
                std::string sid = s.getId();
                if (lowName.find(lowSearch) == std::string::npos && sid.find(search) == std::string::npos)
                    continue;                                    // 两者都不匹配则跳过
            }
            // 按班级筛选
            if (!cls.empty() && s.getClassName() != cls) continue;
            filtered.push_back(s);
        }

        // 分页截取
        int total = (int)filtered.size();
        int start = (page - 1) * size;
        int end = start + size;
        if (end > total) end = total;                            // 防止越界

        // 拼装返回的 JSON（前端期望 d.data 直接是学生数组）
        std::string data = "[";
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
        return utils::jsonResponse(true, "ok", data);
    }

    // ===================================================================
    // 【添加学生】处理 POST /api/students
    // 输入：body = {"id":"学号","name":"姓名","class":"班级","gender":"性别","birthday":"生日"}
    // 会检查学号是否重复，每行还会自动记录 created_at 创建时间
    // ===================================================================
    std::string addStudent(const std::string& body) {
        std::string sid = parseBodyField(body, "id");
        std::string name = parseBodyField(body, "name");
        std::string className = parseBodyField(body, "class");
        std::string gender = parseBodyField(body, "gender");
        std::string birthday = parseBodyField(body, "birthday");

        if (sid.empty()) return utils::errorResponse("学号不能为空");

        std::vector<std::vector<std::string>> all = storage::getStudents();
        for (size_t i = 0; i < all.size(); i++) {
            if (all[i][0] == sid) return utils::errorResponse("学号已存在"); // 查重
        }
        // CSV列顺序：学号, 姓名, 生日, 班级, 性别, 创建时间
        std::vector<std::string> row;
        row.push_back(sid);
        row.push_back(name);
        row.push_back(birthday);
        row.push_back(className);
        row.push_back(gender);
        row.push_back(utils::getCurrentTime());                 // 自动记录创建时间
        all.push_back(row);
        storage::saveStudents(all);
        return utils::jsonResponse(true, "学生添加成功", "{}");
    }

    // ===================================================================
    // 【更新学生信息】处理 PUT /api/students/{学号}
    // 与用户更新逻辑一致：只更新 body 中提供了的字段（字符串非空），未提供的字段保持原值
    // ===================================================================
    std::string updateStudent(const std::string& id, const std::string& body) {
        std::vector<std::vector<std::string>> all = storage::getStudents();
        for (size_t i = 0; i < all.size(); i++) {
            if (all[i][0] == id) {                              // 按学号定位
                std::string name = parseBodyField(body, "name");
                std::string birthday = parseBodyField(body, "birthday");
                std::string className = parseBodyField(body, "class");
                std::string gender = parseBodyField(body, "gender");
                if (!name.empty()) all[i][1] = name;            // 非空才更新
                if (!birthday.empty()) all[i][2] = birthday;
                if (!className.empty()) all[i][3] = className;
                if (!gender.empty()) all[i][4] = gender;
                storage::saveStudents(all);
                return utils::jsonResponse(true, "学生信息更新成功", "{}");
            }
        }
        return utils::errorResponse("学生不存在");
    }

    // ===================================================================
    // 【删除学生】处理 DELETE /api/students/{学号}
    // ===================================================================
    std::string deleteStudent(const std::string& id) {
        if (genericDelete("students", id) == "notfound")
            return utils::errorResponse("学生不存在");
        return utils::jsonResponse(true, "学生已删除", "{}");
    }

    // ===== 班级管理 =====

    // ===================================================================
    // 【获取所有班级】处理 GET /api/classes
    // 返回班级数组，每个班级包含 id, name, grade 三个字段
    // ===================================================================
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

    // ===================================================================
    // 【添加班级】处理 POST /api/classes
    // 输入：body = {"name":"班级名称","grade":"年级"}
    // 班级ID 自动生成，格式为 "CLS" + 序号（如 CLS1, CLS2...）
    // ===================================================================
    std::string addClass(const std::string& body) {
        std::string name = parseBodyField(body, "name");
        std::string grade = parseBodyField(body, "grade");
        if (name.empty()) return utils::errorResponse("班级名称不能为空");

        std::vector<std::vector<std::string>> all = storage::getClasses();
        for (size_t i = 0; i < all.size(); i++) {
            if (all[i][1] == name) return utils::errorResponse("班级已存在"); // 通过名称查重
        }

        std::string newId = "CLS" + std::to_string(all.size() + 1); // 自动生成ID
        // CSV列顺序：ID, 名称, 年级, 创建时间
        std::vector<std::string> row;
        row.push_back(newId);
        row.push_back(name);
        row.push_back(grade);
        row.push_back(utils::getCurrentTime());
        all.push_back(row);
        storage::saveClasses(all);
        return utils::jsonResponse(true, "班级添加成功", "{}");
    }

    // ===================================================================
    // 【更新班级】处理 PUT /api/classes/{班级ID}
    // 只更新 body 中提供的字段（name 或 grade）
    // ===================================================================
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

    // ===================================================================
    // 【删除班级】处理 DELETE /api/classes/{班级ID}
    // ===================================================================
    std::string deleteClass(const std::string& id) {
        if (genericDelete("classes", id) == "notfound")
            return utils::errorResponse("班级不存在");
        return utils::jsonResponse(true, "班级已删除", "{}");
    }

    // ===== 科目管理 =====

    // ===================================================================
    // 【获取所有科目】处理 GET /api/subjects
    // 返回科目数组：[{id, name, full_score}, ...]
    // full_score 为满分值，默认 100，可自定义（如语文150分）
    // ===================================================================
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

    // ===================================================================
    // 【添加科目】处理 POST /api/subjects
    // 输入：body = {"name":"科目名称","full_score":150}
    // 若未提供满分值，默认取 100 分
    // 科目ID 自动生成，格式 "SUB" + 序号
    // ===================================================================
    std::string addSubject(const std::string& body) {
        std::string name = parseBodyField(body, "name");
        std::string fullScoreStr = parseBodyField(body, "full_score");
        int fullScore = 100;
        try { if (!fullScoreStr.empty()) fullScore = std::stoi(fullScoreStr); } catch (...) {}
        if (name.empty()) return utils::errorResponse("科目名称不能为空");

        std::vector<std::vector<std::string>> all = storage::getSubjects();
        for (size_t i = 0; i < all.size(); i++) {
            if (all[i][1] == name) return utils::errorResponse("科目已存在");
        }

        std::string newId = "SUB" + std::to_string(all.size() + 1);
        // CSV列顺序：ID, 名称, 满分
        std::vector<std::string> row;
        row.push_back(newId);
        row.push_back(name);
        row.push_back(std::to_string(fullScore));
        all.push_back(row);
        storage::saveSubjects(all);
        return utils::jsonResponse(true, "科目添加成功", "{}");
    }

    // ===================================================================
    // 【更新科目】处理 PUT /api/subjects/{科目ID}
    // 只更新 body 中提供的字段（name 或 full_score），未提供的字段保持原值
    // ===================================================================
    std::string updateSubject(const std::string& id, const std::string& body) {
        std::vector<std::vector<std::string>> all = storage::getSubjects();
        for (size_t i = 0; i < all.size(); i++) {
            if (all[i][0] == id) {
                std::string name = parseBodyField(body, "name");
                std::string fullScoreStr = parseBodyField(body, "full_score");
                if (!name.empty()) all[i][1] = name;
                if (!fullScoreStr.empty()) all[i][2] = fullScoreStr;
                storage::saveSubjects(all);
                return utils::jsonResponse(true, "科目更新成功", "{}");
            }
        }
        return utils::errorResponse("科目不存在");
    }

    // ===================================================================
    // 【删除科目】处理 DELETE /api/subjects/{科目ID}
    // ===================================================================
    std::string deleteSubject(const std::string& id) {
        if (genericDelete("subjects", id) == "notfound")
            return utils::errorResponse("科目不存在");
        return utils::jsonResponse(true, "科目已删除", "{}");
    }

}
