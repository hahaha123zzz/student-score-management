#include "handlers.h"
#include "models.h"
#include "storage.h"
#include "utils.h"
#include <sstream>

namespace handlers {

    // ===== 考试管理 =====

    // ===================================================================
    // 【获取考试列表】处理 GET /api/exams?status=draft/published/locked
    //
    // 返回每个考试的完整信息，包括：
    //   - 基本字段：id, name, date, status
    //   - subjects 数组：该考试包含的所有科目
    //   - weight_config 对象：每科的满分值和权重
    //     格式：{"语文":{"full":150,"weight":1.0}, "数学":{"full":150,"weight":1.0}}
    //
    // 可选参数 status 用于过滤（草稿/已发布/已锁定）
    // ===================================================================
    std::string getExams(const std::string& queryString) {
        std::string status = utils::getQueryParam(queryString, "status");
        std::vector<std::vector<std::string>> all = storage::getExams();
        std::string data = "[";
        bool first = true;
        for (size_t i = 0; i < all.size(); i++) {
            Exam e = Exam::fromCSV(all[i]);
            if (!status.empty() && e.getStatus() != status) continue; // 状态过滤
            if (!first) data += ","; first = false;
            data += "{";
            data += "\"id\":\"" + utils::jsonEscape(e.getId()) + "\",";
            data += "\"name\":\"" + utils::jsonEscape(e.getName()) + "\",";
            data += "\"date\":\"" + utils::jsonEscape(e.getDate()) + "\",";
            data += "\"status\":\"" + utils::jsonEscape(e.getStatus()) + "\",";
            data += "\"subjects\":[";
            // subjects 以竖线分隔存储，如 "语文|数学|英语"
            std::vector<std::string> slist = e.getSubjectList();
            for (size_t j = 0; j < slist.size(); j++) {
                if (j > 0) data += ",";
                data += "\"" + utils::jsonEscape(slist[j]) + "\"";
            }
            data += "],";
            data += "\"weight_config\":{";
            // weight_config 以竖线分隔，格式为 "科目名:满分:权重|科目名:满分:权重"
            std::vector<std::string> configs = utils::split(e.getWeightConfig(), '|');
            for (size_t j = 0; j < configs.size(); j++) {
                std::vector<std::string> parts = utils::split(configs[j], ':'); // 用冒号拆出三部分
                if (parts.size() >= 3) {
                    if (j > 0) data += ",";
                    data += "\"" + utils::jsonEscape(parts[0]) + "\":{";
                    data += "\"full\":" + parts[1] + ",";     // 满分值
                    data += "\"weight\":" + parts[2];          // 权重系数
                    data += "}";
                }
            }
            data += "}";
            data += "}";
        }
        data += "]";
        return utils::jsonResponse(true, "ok", data);
    }

    // ===================================================================
    // 【创建考试】处理 POST /api/exams
    // 输入：body = {"name":"期中考试","date":"2024-06-15"}
    // 默认包含语文、数学、英语三科，满分各150，权重各1.0
    // 初始状态为 "draft"（草稿），学生端不可见
    // ===================================================================
    std::string addExam(const std::string& body) {
        std::string name = parseBodyField(body, "name");
        std::string date = parseBodyField(body, "date");
        if (name.empty()) return utils::errorResponse("考试名称不能为空");

        std::vector<std::vector<std::string>> all = storage::getExams();
        std::string newId = "EXAM" + std::to_string(all.size() + 1);

        // 默认科目和权重配置
        std::string subjectsStr = "语文|数学|英语";               // 竖线分隔的科目列表
        std::string weightConfig = "语文:150:1.0|数学:150:1.0|英语:150:1.0"; // 科目:满分:权重

        // CSV列顺序：ID, 名称, 日期, 状态, 科目列表, 权重配置
        std::vector<std::string> row;
        row.push_back(newId);
        row.push_back(name);
        row.push_back(date);
        row.push_back("draft");                                  // 初始状态为草稿
        row.push_back(subjectsStr);
        row.push_back(weightConfig);
        all.push_back(row);
        storage::saveExams(all);
        return utils::jsonResponse(true, "考试创建成功", "{}");
    }

    // ===================================================================
    // 【更新考试】处理 PUT /api/exams/{考试ID}
    // 只更新 body 中提供的字段（name 或 date）
    // ===================================================================
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

    // ===================================================================
    // 【删除考试】处理 DELETE /api/exams/{考试ID}
    // ===================================================================
    std::string deleteExam(const std::string& id) {
        if (genericDelete("exams", id) == "notfound")
            return utils::errorResponse("考试不存在");
        return utils::jsonResponse(true, "考试已删除", "{}");
    }

    // ===================================================================
    // 【发布成绩】处理 PUT /api/exams/{考试ID}/publish
    // 将考试状态从 "draft" 改为 "published"
    //
    // ★重要：状态机逻辑 —— 考试有三种状态
    //   draft     → 草稿，学生端不可见，管理员可编辑
    //   published → 已发布，学生可查看成绩
    //   locked    → 已锁定，任何人不可修改，也不能发布/撤回
    //
    // 已锁定的考试不能再发布，这是数据保护机制：
    //   locked 状态是最终状态，一旦锁定就"封存"了
    // ===================================================================
    std::string publishExam(const std::string& id) {
        std::vector<std::vector<std::string>> all = storage::getExams();
        for (size_t i = 0; i < all.size(); i++) {
            if (all[i][0] == id) {
                if (all[i][3] == "locked") return utils::errorResponse("已锁定不能发布"); // 锁定状态不允许发布
                all[i][3] = "published";                       // 修改第4列（索引3）的状态字段
                storage::saveExams(all);
                return utils::jsonResponse(true, "成绩已发布", "{}");
            }
        }
        return utils::errorResponse("考试不存在");
    }

    // ===================================================================
    // 【撤回成绩】处理 PUT /api/exams/{考试ID}/retract
    // 将考试状态从 "published" 改回 "draft"
    // 撤回后学生端不再可见，管理员可以继续编辑成绩
    // 同样，locked 状态不可撤回
    // ===================================================================
    std::string retractExam(const std::string& id) {
        std::vector<std::vector<std::string>> all = storage::getExams();
        for (size_t i = 0; i < all.size(); i++) {
            if (all[i][0] == id) {
                if (all[i][3] == "locked")
                    return utils::errorResponse("已锁定不能撤回，请先解锁");
                if (all[i][3] != "published")
                    return utils::errorResponse("只有已发布的考试才能撤回");
                all[i][3] = "draft";                          // 改回草稿状态
                storage::saveExams(all);
                return utils::jsonResponse(true, "成绩已撤回", "{}");
            }
        }
        return utils::errorResponse("考试不存在");
    }

    std::string unlockExam(const std::string& id) {
        std::vector<std::vector<std::string>> all = storage::getExams();
        for (size_t i = 0; i < all.size(); i++) {
            if (all[i][0] == id) {
                if (all[i][3] != "locked")
                    return utils::errorResponse("只有已锁定的考试才能解锁");
                all[i][3] = "draft";
                storage::saveExams(all);
                return utils::jsonResponse(true, "成绩已解锁，状态恢复为草稿", "{}");
            }
        }
        return utils::errorResponse("考试不存在");
    }

    // ===== 成绩管理 =====

    // ===================================================================
    // 【录入/修改成绩】处理 POST /api/grades
    // 输入：body = {"exam_id":"考试ID", "student_id":"学号", "scores":"85,92,78"}
    //
    // ★★★ 覆盖逻辑详解（这是成绩管理的核心）★★★
    //   scores 字段是一个用逗号分隔的字符串，如 "85,92,78"
    //   对应考试中每个科目的分数（按科目顺序排列）
    //
    //   录入流程：
    //     ① 解析 body 中的 exam_id, student_id, scores
    //     ② 在 grades.csv 中查找 学号+考试ID 同时匹配的记录
    //     ③ 如果找到了 → 直接覆盖该行的 scores 列（修改现有成绩）
    //     ④ 如果没找到 → 新增一行成绩记录（首次录入）
    //
    //   本质是"存在即更新，不存在即插入"（即数据库中的 UPSERT 操作）
    //   这意味着重复提交同一个学生的成绩不会产生多条记录，总是保持一科一考一条
    // ===================================================================
    std::string setGrades(const std::string& body) {
        std::string examId = parseBodyField(body, "exam_id");
        std::string studentId = parseBodyField(body, "student_id");
        if (examId.empty() || studentId.empty())
            return utils::errorResponse("考试ID和学生ID不能为空");

        // 手动提取 scores JSON 对象 {"语文":85,"数学":92} → 转为 "85|92" 存储
        std::string scores;
        size_t scoresPos = body.find("\"scores\":{");
        if (scoresPos != std::string::npos) {
            scoresPos += 10; // 跳过 "scores":{
            // 用括号计数找到配对的 }
            int braceCount = 1;
            size_t j = scoresPos;
            while (j < body.size() && braceCount > 0) {
                if (body[j] == '{') braceCount++;
                else if (body[j] == '}') braceCount--;
                j++;
            }
            std::string scoresObj = body.substr(scoresPos, j - scoresPos - 1); // 去掉末尾的 }
            // 解析 key:value 对（忽略 key，只取 value）
            size_t k = 0;
            bool firstVal = true;
            while (k < scoresObj.size()) {
                // 找冒号
                size_t colon = scoresObj.find(':', k);
                if (colon == std::string::npos) break;
                k = colon + 1;
                // 跳过空白
                while (k < scoresObj.size() && (scoresObj[k] == ' ' || scoresObj[k] == '\t')) k++;
                // 读取数字值
                size_t start = k;
                while (k < scoresObj.size() && scoresObj[k] != ',' && scoresObj[k] != '}') k++;
                std::string val = scoresObj.substr(start, k - start);
                if (!firstVal) scores += "|";
                firstVal = false;
                scores += val;
                if (k < scoresObj.size() && scoresObj[k] == ',') k++;
            }
        }

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

    // ===================================================================
    // 【CSV导入成绩】处理 POST /api/grades/import
    // 当前为预留接口，返回"开发中"提示
    // ===================================================================
    std::string importCSV(const std::string& body) {
        std::string examId = parseBodyField(body, "exam_id");
        std::string csvContent = parseBodyField(body, "csv");
        if (examId.empty() || csvContent.empty())
            return utils::errorResponse("考试ID和CSV内容不能为空");

        std::vector<std::vector<std::string>> exams = storage::getExams();
        std::vector<std::string> subjects;
        for (size_t i = 0; i < exams.size(); i++) {
            if (exams[i][0] == examId) {
                subjects = utils::split(exams[i][4], '|');
                break;
            }
        }
        if (subjects.empty()) return utils::errorResponse("考试不存在");

        std::istringstream stream(csvContent);
        std::string line;
        std::getline(stream, line);

        std::vector<std::vector<std::string>> grades = storage::getGrades();

        int imported = 0, failed = 0;
        while (std::getline(stream, line)) {
            if (line.empty()) continue;

            std::vector<std::string> parts;
            std::string current;
            for (size_t i = 0; i < line.size(); i++) {
                if (line[i] == ',') { parts.push_back(current); current.clear(); }
                else current += line[i];
            }
            parts.push_back(current);

            if (parts.size() < 3) { failed++; continue; }

            std::string studentId = parts[0];
            std::string scores;
            for (size_t i = 2; i < parts.size() && (i - 2) < subjects.size(); i++) {
                if (i > 2) scores += "|";
                scores += parts[i];
            }

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
                row.push_back("import");
                row.push_back(utils::getCurrentTime());
                grades.push_back(row);
            }
            imported++;
        }
        storage::saveGrades(grades);

        std::string result = "{\"imported\":" + std::to_string(imported) + ",\"failed\":" + std::to_string(failed) + "}";
        return utils::jsonResponse(true, "导入完成", result);
    }

    // ===================================================================
    // 【CSV导出成绩】处理 GET /api/grades/export?exam_id=考试ID
    //
    // 业务流程：
    //   ① 根据 exam_id 找到考试信息，取出科目列表
    //   ② 生成 CSV 表头：学号,姓名,科目1,科目2,...
    //   ③ 遍历成绩表，筛选该考试的成绩记录
    //   ④ 通过学号关联学生表，取出学生姓名
    //   ⑤ 将 scores 字段（逗号分隔）拆分为各科分数，填入对应列
    //
    // 返回 JSON：{"success":true, "data":{"csv":"学号,姓名,...\\r\\n001,张三,..."}}
    // 前端拿到 csv 字符串后可保存为 .csv 文件下载
    // ===================================================================
    std::string exportCSV(const std::string& queryString) {
        std::string examId = utils::getQueryParam(queryString, "exam_id");
        if (examId.empty()) return utils::errorResponse("请指定考试ID");

        // ① 找到考试，获取科目列表
        std::vector<std::vector<std::string>> exams = storage::getExams();
        std::string subjects;
        for (size_t i = 0; i < exams.size(); i++) {
            if (exams[i][0] == examId) { subjects = exams[i][4]; break; }
        }
        if (subjects.empty()) return utils::errorResponse("考试不存在");

        std::vector<std::vector<std::string>> grades = storage::getGrades();
        std::vector<std::vector<std::string>> students = storage::getStudents();

        std::vector<std::string> subList = utils::split(subjects, '|'); // 拆分科目列
        // ② 构造CSV表头行
        std::string csv = "学号,姓名";
        for (size_t i = 0; i < subList.size(); i++) csv += "," + subList[i];
        csv += "\r\n";                                        // Windows换行符

        // ③④⑤ 遍历成绩，逐行输出
        for (size_t i = 0; i < grades.size(); i++) {
            if (grades[i][2] != examId) continue;             // 只处理该考试的成绩
            std::string sid = grades[i][1];                   // 学号
            csv += sid + ",";
            // ④ 查找学生姓名
            std::string sname = sid;
            for (size_t j = 0; j < students.size(); j++) {
                if (students[j][0] == sid) { sname = students[j][1]; break; }
            }
            csv += sname;

            // ⑤ 拆分scores填入各科目列
            Grade g = Grade::fromCSV(grades[i]);
            std::vector<double> scoreList = g.getScoreList();  // 如 [85, 92, 78]
            for (size_t j = 0; j < subList.size(); j++) {
                csv += ",";
                if (j < scoreList.size())
                    csv += std::to_string((int)scoreList[j]);   // 转为整数显示
            }
            csv += "\r\n";
        }

        std::string data = "{\"csv\":\"" + utils::jsonEscape(csv) + "\"}";
        return utils::jsonResponse(true, "ok", data);
    }

    // ===================================================================
    // 【锁定成绩】处理 POST /api/grades/lock/{考试ID}
    // 将考试状态改为 "locked"，此后该考试的成绩不可再修改、发布或撤回
    // 用于期末成绩归档等场景
    // ===================================================================
    std::string lockGrades(const std::string& examId) {
        std::vector<std::vector<std::string>> all = storage::getExams();
        for (size_t i = 0; i < all.size(); i++) {
            if (all[i][0] == examId) {
                all[i][3] = "locked";                         // 最终状态
                storage::saveExams(all);
                return utils::jsonResponse(true, "成绩已锁定", "{}");
            }
        }
        return utils::errorResponse("考试不存在");
    }

    // ===================================================================
    // 【查询成绩列表】处理 GET /api/grades?exam_id=考试ID&student_id=学号（可选）
    //
    // 参数均为可选：
    //   exam_id    —— 按考试筛选
    //   student_id —— 按学号筛选
    //   两个参数都不传时，返回所有成绩
    //
    // 返回的每条成绩还关联了学生姓名和班级信息（通过学号关联 students 表查询）
    // 每条成绩对象包含：student_id, exam_id, student_name, class, scores, is_makeup
    // ===================================================================
    std::string getGrades(const std::string& queryString) {
        std::string examId = utils::getQueryParam(queryString, "exam_id");
        std::string studentId = utils::getQueryParam(queryString, "student_id");

        std::vector<std::vector<std::string>> grades = storage::getGrades();
        std::vector<std::vector<std::string>> exams = storage::getExams();

        std::string data = "[";
        bool first = true;
        for (size_t i = 0; i < grades.size(); i++) {
            if (!examId.empty() && grades[i][2] != examId) continue;
            if (!studentId.empty() && grades[i][1] != studentId) continue;
            if (!first) data += ","; first = false;

            std::string info = findStudentInfo(grades[i][1]);
            std::string sname = utils::split(info, ',')[0];
            std::string sclass = utils::split(info, ',')[1];

            // 查找考试以获取科目列表
            std::vector<std::string> subList;
            for (size_t j = 0; j < exams.size(); j++) {
                if (exams[j][0] == grades[i][2]) {
                    subList = utils::split(exams[j][4], '|');
                    break;
                }
            }

            // 展开 scores: "85|92|78" → {"语文":85,"数学":92,"英语":78}
            std::vector<double> scoreList = Grade::fromCSV(grades[i]).getScoreList();
            std::string scoresObj = "{";
            for (size_t j = 0; j < scoreList.size() && j < subList.size(); j++) {
                if (j > 0) scoresObj += ",";
                scoresObj += "\"" + utils::jsonEscape(subList[j]) + "\":" + std::to_string(scoreList[j]);
            }
            scoresObj += "}";

            data += "{";
            data += "\"student_id\":\"" + utils::jsonEscape(grades[i][1]) + "\",";
            data += "\"exam_id\":\"" + utils::jsonEscape(grades[i][2]) + "\",";
            data += "\"student_name\":\"" + utils::jsonEscape(sname) + "\",";
            data += "\"class\":\"" + utils::jsonEscape(sclass) + "\",";
            data += "\"scores\":" + scoresObj + ",";
            data += "\"is_makeup\":" + std::string(grades[i][4] == "true" ? "true" : "false");
            data += "}";
        }
        data += "]";
        return utils::jsonResponse(true, "ok", data);
    }

    // ===================================================================
    // 【查询学生所有成绩】处理 GET /api/grades/{学号}
    // 内部直接调用 getGrades("student_id=学号")，复用成绩查询逻辑
    // ===================================================================
    std::string getStudentGrades(const std::string& studentId) {
        return getGrades("student_id=" + studentId);
    }

    // ===================================================================
    // 【录入补考成绩】处理 POST /api/grades/makeup
    // 输入：body = {"student_id":"学号", "exam_id":"考试ID", "scores":"补考分数,..."}
    //
    // 与普通 setGrades 的区别：
    //   ① 补考成绩必须基于已有的成绩记录（学生必须先有正常考试成绩才能录入补考）
    //   ② 录入后 is_makeup 字段标记为 "true"，前端可据此显示"补考"标签
    //   ③ 如果找不到原有成绩记录，返回错误"未找到该学生的成绩记录"
    //   ④ 补考的 scores 直接覆盖原有成绩
    // ===================================================================
    std::string markMakeup(const std::string& body) {
        std::string studentId = parseBodyField(body, "student_id");
        std::string examId = parseBodyField(body, "exam_id");
        std::string scores = parseBodyField(body, "scores");

        std::vector<std::vector<std::string>> grades = storage::getGrades();
        bool found = false;
        for (size_t i = 0; i < grades.size(); i++) {
            if (grades[i][1] == studentId && grades[i][2] == examId) {
                grades[i][3] = scores;                       // 覆盖为补考成绩
                grades[i][4] = "true";                       // 标记为补考
                found = true;
                break;
            }
        }
        if (!found) return utils::errorResponse("未找到该学生的成绩记录"); // 必须有原始记录
        storage::saveGrades(grades);
        return utils::jsonResponse(true, "补考成绩已录入", "{}");
    }

}
