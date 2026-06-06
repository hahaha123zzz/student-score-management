/**
 * stats.cpp — 统计分析模块实现
 * ==============================
 * 本文件包含：排名计算、班级对比、分数段分布、成绩趋势、不及格预警、入学统计。
 *
 * 核心思路：
 *   1. 从 storage 模块读取 CSV 数据（学生、成绩、考试）
 *   2. 根据查询参数筛选数据
 *   3. 使用手写排序（sort 模块）、遍历、计数等基本算法完成统计
 *   4. 手写 JSON 字符串返回给前端
 *
 * 依赖关系：storage → models → sort → utils
 */
#include "stats.h"
#include "handlers.h"
#include "storage.h"
#include "models.h"
#include "sort.h"
#include "utils.h"
#include <map>

namespace stats {

    /**
     * 【排名算法】
     * 根据考试ID和类型，计算学生的成绩排名。
     *
     * 参数（通过 queryString 传入）：
     *   exam_id — 考试ID（必填）
     *   type    — "class"=班级内排名, 其他=全年级排名
     *   subject — 空=按总分排, "数学"=按单科排
     *
     * 核心算法步骤：
     *   1. 遍历所有成绩，筛选出指定考试的成绩
     *   2. 如果指定了科目，取该科原始分；否则用加权总分
     *   3. 用快速排序把分数从高到低排序
     *   4. 依次赋予名次 1, 2, 3...
     *   5. 如果是班级排名，再按班级分组，每个班内部重新编号
     */
    std::string getRank(const std::string& queryString) {
        // 从URL参数中解析查询条件
        std::string examId = utils::getQueryParam(queryString, "exam_id");
        std::string type = utils::getQueryParam(queryString, "type");
        std::string subject = utils::getQueryParam(queryString, "subject");

        if (examId.empty()) return utils::errorResponse("请指定考试ID");

        // 加载需要的数据
        std::vector<std::vector<std::string>> grades = storage::getGrades();
        std::vector<std::vector<std::string>> exams = storage::getExams();

        // 找到对应的考试（获取科目列表和权重配置）
        Exam exam;
        for (size_t i = 0; i < exams.size(); i++) {
            if (exams[i][0] == examId) {
                exam = Exam::fromCSV(exams[i]);
                break;
            }
        }

        // 这5个vector配合使用：scores[i]是第i个学生的分数，
        // indices[i]是该学生在原始成绩数组中的位置
        std::vector<double> scores;           // 每个学生的分数
        std::vector<int> indices;             // 分数排序后对应的原始位置
        std::vector<std::string> studentIds;  // 学号
        std::vector<std::string> studentNames; // 姓名
        std::vector<std::string> classes;      // 班级

        // 遍历成绩表，收集参与排名的数据
        for (size_t i = 0; i < grades.size(); i++) {
            if (grades[i][2] != examId) continue; // 只处理该考试的成绩
            std::string sid = grades[i][1];
            Grade g = Grade::fromCSV(grades[i]);

            double score;
            if (!subject.empty()) {
                // 单科排名：取该科目原始分
                std::vector<std::string> subList = exam.getSubjectList();
                score = 0;
                for (size_t j = 0; j < subList.size(); j++) {
                    if (subList[j] == subject) {
                        score = g.getScoreByIndex((int)j);
                        break;
                    }
                }
            } else {
                // 总分排名：计算加权总分（各科按权重折算后相加）
                score = g.calcTotal(exam.getWeightConfig());
            }

            scores.push_back(score);
            indices.push_back((int)i);  // 记录在原始数组中的索引
            studentIds.push_back(sid);

            // 根据学号查学生信息
            std::string info = handlers::findStudentInfo(sid);
            std::string sname = utils::split(info, ',')[0];
            std::string sclass = utils::split(info, ',')[1];
            studentNames.push_back(sname);
            classes.push_back(sclass);
        }

        // 使用手写的快速排序（从高到低），indices数组跟随调整
        sort::quickSortWithIndex(scores, indices);

        // --- 班级排名模式：按班级分组，每个班内部独立排名 ---
        if (type == "class") {
            // classJsons: 班级名 -> 该班排名JSON片段
            // classRanks:  班级名 -> 当前排名编号（从1开始递增）
            std::map<std::string, std::string> classJsons;
            std::map<std::string, int> classRanks;

            for (size_t i = 0; i < indices.size(); i++) {
                int idx = indices[i];
                std::string cls = classes[idx];

                // 每个班第一次遇到时，初始化排名编号为1
                if (classJsons[cls].empty()) classRanks[cls] = 1;

                std::string& cjson = classJsons[cls];
                if (!cjson.empty()) cjson += ","; // 不是第一条，加逗号分隔
                cjson += "{\"rank\":" + std::to_string(classRanks[cls]++) + ",";
                cjson += "\"student_name\":\"" + utils::jsonEscape(studentNames[idx]) + "\",";
                cjson += "\"student_id\":\"" + utils::jsonEscape(studentIds[idx]) + "\",";
                cjson += "\"total_score\":" + std::to_string((int)scores[i]) + "}";
            }

            // 组装按班级分组的JSON对象
            std::string data = "{";
            bool firstCls = true;
            for (std::map<std::string, std::string>::iterator it = classJsons.begin();
                 it != classJsons.end(); ++it) {
                if (!firstCls) data += ","; firstCls = false;
                data += "\"" + utils::jsonEscape(it->first) + "\":[" + it->second + "]";
            }
            data += "}";
            return utils::jsonResponse(true, "ok", data);
        }

        // --- 全年级排名模式：所有学生一起排 ---
        std::string data = "[";
        for (size_t i = 0; i < indices.size(); i++) {
            int idx = indices[i];
            if (i > 0) data += ",";
            data += "{";
            data += "\"rank\":" + std::to_string((int)i + 1) + ",";       // 名次从1开始
            data += "\"student_id\":\"" + utils::jsonEscape(studentIds[idx]) + "\",";
            data += "\"student_name\":\"" + utils::jsonEscape(studentNames[idx]) + "\",";
            data += "\"class\":\"" + utils::jsonEscape(classes[idx]) + "\",";
            data += "\"total_score\":" + std::to_string((int)scores[i]);
            data += "}";
        }
        data += "]";
        return utils::jsonResponse(true, "ok", data);
    }

    /**
     * 【班级对比分析】
     * 统计每个班级在某次考试中的表现指标。
     *
     * 统计指标包括：
     *   count   — 参考人数
     *   average — 平均分 = 总分 / 人数
     *   excellent_rate — 优秀率（≥90分的学生百分比）
     *   good_rate — 良好率（≥80分）
     *   pass_rate — 及格率（≥60分）
     *   max / min — 最高分 / 最低分
     *   median — 中位数（排序后中间位置的值）
     */
    std::string getClassCompare(const std::string& queryString) {
        std::string examId = utils::getQueryParam(queryString, "exam_id");
        if (examId.empty()) return utils::errorResponse("请指定考试ID");

        std::vector<std::vector<std::string>> grades = storage::getGrades();
        std::vector<std::vector<std::string>> students = storage::getStudents();
        std::vector<std::vector<std::string>> exams = storage::getExams();

        Exam exam;
        for (size_t i = 0; i < exams.size(); i++) {
            if (exams[i][0] == examId) { exam = Exam::fromCSV(exams[i]); break; }
        }

        // 用map把分数按班级分组：班级名 -> 该班所有学生的加权总分列表
        std::map<std::string, std::vector<double>> classScores;
        // 先为每个班级创建空的分数列表
        for (size_t i = 0; i < students.size(); i++) {
            std::string cls = students[i][3];
            classScores[cls] = std::vector<double>();
        }

        // 遍历成绩，把加权总分放入对应班级的列表
        for (size_t i = 0; i < grades.size(); i++) {
            if (grades[i][2] != examId) continue;
            std::string sid = grades[i][1];
            std::string cls;
            for (size_t j = 0; j < students.size(); j++) {
                if (students[j][0] == sid) { cls = students[j][3]; break; }
            }
            if (cls.empty()) continue;
            Grade g = Grade::fromCSV(grades[i]);
            double total = g.calcTotal(exam.getWeightConfig());
            classScores[cls].push_back(total);
        }

        // 对每个班级计算统计指标
        std::string data = "[";
        bool first = true;
        for (std::map<std::string, std::vector<double>>::iterator it = classScores.begin();
             it != classScores.end(); ++it) {
            std::vector<double>& scores = it->second;
            if (scores.empty()) continue; // 该班无人参加考试，跳过
            if (!first) data += ","; first = false;

            // 累加求和 + 统计各档人数
            double sum = 0;
            int excellent = 0, good = 0, passNum = 0;
            for (size_t i = 0; i < scores.size(); i++) {
                sum += scores[i];
                if (scores[i] >= 90) excellent++;   // 优秀
                if (scores[i] >= 80) good++;        // 良好
                if (scores[i] >= 60) passNum++;     // 及格
            }

            // 排序后取最值和中位数
            sort::quickSort(scores);
            int n = (int)scores.size();

            data += "{";
            data += "\"class\":\"" + utils::jsonEscape(it->first) + "\",";
            data += "\"count\":" + std::to_string(n) + ",";
            data += "\"average\":" + std::to_string(utils::round2(sum / n)) + ",";
            data += "\"excellent_rate\":" + std::to_string(utils::round2((double)excellent / n * 100)) + ",";
            data += "\"good_rate\":" + std::to_string(utils::round2((double)good / n * 100)) + ",";
            data += "\"pass_rate\":" + std::to_string(utils::round2((double)passNum / n * 100)) + ",";
            data += "\"max\":" + std::to_string(utils::round2(scores.back())) + ",";
            data += "\"min\":" + std::to_string(utils::round2(scores[0])) + ",";
            data += "\"median\":" + std::to_string(utils::round2(scores[n / 2]));
            data += "}";
        }
        data += "]";
        return utils::jsonResponse(true, "ok", data);
    }

    /**
     * 【分数段分布统计】
     * 把成绩分成5个分数段，统计每个段有多少人。
     *
     * 分段规则：
     *   <60    — 不及格
     *   60-69  — 及格
     *   70-79  — 中等
     *   80-89  — 良好
     *   90-100 — 优秀
     *
     * 可以按总分统计（subject为空），也可以按单科统计。
     */
    std::string getDistribution(const std::string& queryString) {
        std::string examId = utils::getQueryParam(queryString, "exam_id");
        std::string subject = utils::getQueryParam(queryString, "subject");
        if (examId.empty()) return utils::jsonResponse(true, "ok", "[]");

        std::vector<std::vector<std::string>> grades = storage::getGrades();
        std::vector<std::vector<std::string>> exams = storage::getExams();

        Exam exam;
        for (size_t i = 0; i < exams.size(); i++) {
            if (exams[i][0] == examId) { exam = Exam::fromCSV(exams[i]); break; }
        }

        // 5个计数器，分别对应5个分数段
        int ranges[5] = {0, 0, 0, 0, 0};
        const char* labels[5] = {"<60", "60-69", "70-79", "80-89", "90-100"};

        for (size_t i = 0; i < grades.size(); i++) {
            if (grades[i][2] != examId) continue;
            Grade g = Grade::fromCSV(grades[i]);

            double score;
            if (!subject.empty()) {
                // 单科分需要归一化到百分制（150分满分→转成100分为基准）
                std::vector<std::string> slist = exam.getSubjectList();
                score = 0;
                for (size_t j = 0; j < slist.size(); j++) {
                    if (slist[j] == subject) { score = g.getScoreByIndex((int)j); break; }
                }
                double full = exam.getSubjectFull(subject);
                score = score / full * 100; // 归一化
            } else {
                // 加权总分已经是百分制
                score = g.calcTotal(exam.getWeightConfig());
            }

            // 阶梯判断，归入对应段
            if (score < 60) ranges[0]++;
            else if (score < 70) ranges[1]++;
            else if (score < 80) ranges[2]++;
            else if (score < 90) ranges[3]++;
            else ranges[4]++;
        }

        std::string data = "[";
        for (int i = 0; i < 5; i++) {
            if (i > 0) data += ",";
            data += "{\"range\":\"" + std::string(labels[i]) + "\",\"count\":" + std::to_string(ranges[i]) + "}";
        }
        data += "]";
        return utils::jsonResponse(true, "ok", data);
    }

    /**
     * 【成绩趋势】
     * 查询一个学生在所有考试中的成绩变化，每个考试一个数据点。
     * X轴=考试名称，Y轴=总分 → 可以在前端画折线图。
     */
    std::string getTrend(const std::string& studentId) {
        if (studentId.empty()) return utils::jsonResponse(true, "ok", "[]");

        std::vector<std::vector<std::string>> grades = storage::getGrades();
        std::vector<std::vector<std::string>> exams = storage::getExams();

        std::string data = "[";
        bool first = true;
        for (size_t i = 0; i < grades.size(); i++) {
            if (grades[i][1] != studentId) continue;
            std::string eid = grades[i][2];
            Grade g = Grade::fromCSV(grades[i]);

            Exam exam;
            for (size_t j = 0; j < exams.size(); j++) {
                if (exams[j][0] == eid) { exam = Exam::fromCSV(exams[j]); break; }
            }

            double total = g.calcTotal(exam.getWeightConfig());
            if (!first) data += ","; first = false;
            data += "{";
            data += "\"exam_id\":\"" + utils::jsonEscape(eid) + "\",";
            data += "\"exam_name\":\"" + utils::jsonEscape(exam.getName()) + "\",";
            data += "\"date\":\"" + utils::jsonEscape(exam.getDate()) + "\",";
            data += "\"total_score\":" + std::to_string(utils::round2(total)) + ",";
            data += "\"is_makeup\":" + std::string(g.getIsMakeup() ? "true" : "false");
            data += "}";
        }
        data += "]";
        return utils::jsonResponse(true, "ok", data);
    }

    /**
     * 【不及格预警】
     * 扫描所有非补考成绩，找出不及格（百分比<60%）的科目。
     * 对每个不及格记录返回学生姓名、班级、考试、科目、分数。
     */
    std::string getWarnings() {
        std::vector<std::vector<std::string>> grades = storage::getGrades();
        std::vector<std::vector<std::string>> exams = storage::getExams();

        std::string data = "[";
        bool first = true;
        for (size_t i = 0; i < grades.size(); i++) {
            Grade g = Grade::fromCSV(grades[i]);
            if (g.getIsMakeup()) continue; // 补考成绩不列入预警

            std::string eid = grades[i][2];
            Exam exam;
            for (size_t j = 0; j < exams.size(); j++) {
                if (exams[j][0] == eid) { exam = Exam::fromCSV(exams[j]); break; }
            }

            std::vector<std::string> slist = exam.getSubjectList();
            std::vector<double> scoreList = g.getScoreList();

            // 查学生姓名和班级
            std::string info = handlers::findStudentInfo(grades[i][1]);
            std::string sname = utils::split(info, ',')[0];
            std::string sclass = utils::split(info, ',')[1];

            // 逐科检查是否不及格
            for (size_t j = 0; j < slist.size() && j < scoreList.size(); j++) {
                double full = exam.getSubjectFull(slist[j]);
                if (scoreList[j] / full * 100 < 60) {
                    if (!first) data += ","; first = false;
                    data += "{";
                    data += "\"student_id\":\"" + utils::jsonEscape(grades[i][1]) + "\",";
                    data += "\"student_name\":\"" + utils::jsonEscape(sname) + "\",";
                    data += "\"class\":\"" + utils::jsonEscape(sclass) + "\",";
                    data += "\"exam_name\":\"" + utils::jsonEscape(exam.getName()) + "\",";
                    data += "\"subject\":\"" + utils::jsonEscape(slist[j]) + "\",";
                    data += "\"score\":" + std::to_string(utils::round2(scoreList[j])) + ",";
                    data += "\"full\":" + std::to_string(utils::round2(full)) + ",";
                    data += "\"type\":\"不及格\"";
                    data += "}";
                }
            }
        }
        data += "]";
        return utils::jsonResponse(true, "ok", data);
    }

    /**
     * 【入学统计】
     * 统计每个班级的学生人数。遍历学生表，用map按班级名计数。
     */
    std::string getEnrollmentStats() {
        std::vector<std::vector<std::string>> students = storage::getStudents();
        std::vector<std::vector<std::string>> classes = storage::getClasses();

        // map的键是班级名，值是该班学生人数
        std::map<std::string, int> classCount;
        for (size_t i = 0; i < students.size(); i++) {
            classCount[students[i][3]]++; // students[i][3]是班级字段
        }

        std::string data = "[";
        for (size_t i = 0; i < classes.size(); i++) {
            if (i > 0) data += ",";
            data += "{";
            data += "\"class\":\"" + utils::jsonEscape(classes[i][1]) + "\",";
            data += "\"count\":" + std::to_string(classCount[classes[i][1]]);
            data += "}";
        }
        data += "]";
        return utils::jsonResponse(true, "ok", data);
    }

}
