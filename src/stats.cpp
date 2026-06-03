#include "stats.h"
#include "storage.h"
#include "models.h"
#include "sort.h"
#include "utils.h"
#include <map>

namespace stats {

    std::string getRank(const std::string& queryString) {
        std::string examId = utils::getQueryParam(queryString, "exam_id");
        std::string type = utils::getQueryParam(queryString, "type");
        std::string subject = utils::getQueryParam(queryString, "subject");

        if (examId.empty()) return utils::errorResponse("请指定考试ID");

        std::vector<std::vector<std::string>> grades = storage::getGrades();
        std::vector<std::vector<std::string>> students = storage::getStudents();
        std::vector<std::vector<std::string>> exams = storage::getExams();

        Exam exam;
        for (size_t i = 0; i < exams.size(); i++) {
            if (exams[i][0] == examId) {
                exam = Exam::fromCSV(exams[i]);
                break;
            }
        }

        std::vector<double> scores;
        std::vector<int> indices;
        std::vector<std::string> studentIds;
        std::vector<std::string> studentNames;
        std::vector<std::string> classes;

        for (size_t i = 0; i < grades.size(); i++) {
            if (grades[i][2] != examId) continue;
            std::string sid = grades[i][1];
            Grade g = Grade::fromCSV(grades[i]);

            double score;
            if (!subject.empty()) {
                std::vector<std::string> subList = exam.getSubjectList();
                score = 0;
                for (size_t j = 0; j < subList.size(); j++) {
                    if (subList[j] == subject) {
                        score = g.getScoreByIndex((int)j);
                        break;
                    }
                }
            } else {
                score = g.calcTotal(exam.getWeightConfig());
            }

            scores.push_back(score);
            indices.push_back((int)i);
            studentIds.push_back(sid);

            std::string sname, sclass;
            for (size_t j = 0; j < students.size(); j++) {
                if (students[j][0] == sid) {
                    sname = students[j][1];
                    sclass = students[j][3];
                    break;
                }
            }
            studentNames.push_back(sname);
            classes.push_back(sclass);
        }

        // 手写快速排序（从高到低）
        sort::quickSortWithIndex(scores, indices);

        if (type == "class") {
            std::map<std::string, std::string> classJsons;
            std::map<std::string, int> classRanks;
            for (size_t i = 0; i < indices.size(); i++) {
                int idx = indices[i];
                std::string cls = classes[idx];
                if (classJsons[cls].empty()) classRanks[cls] = 1;
                std::string& cjson = classJsons[cls];
                if (!cjson.empty()) cjson += ",";
                cjson += "{\"rank\":" + std::to_string(classRanks[cls]++) + ",";
                cjson += "\"student_name\":\"" + utils::jsonEscape(studentNames[idx]) + "\",";
                cjson += "\"student_id\":\"" + utils::jsonEscape(studentIds[idx]) + "\",";
                cjson += "\"total_score\":" + std::to_string((int)scores[i]) + "}";
            }
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

        std::string data = "[";
        for (size_t i = 0; i < indices.size(); i++) {
            int idx = indices[i];
            if (i > 0) data += ",";
            data += "{";
            data += "\"rank\":" + std::to_string((int)i + 1) + ",";
            data += "\"student_id\":\"" + utils::jsonEscape(studentIds[idx]) + "\",";
            data += "\"student_name\":\"" + utils::jsonEscape(studentNames[idx]) + "\",";
            data += "\"class\":\"" + utils::jsonEscape(classes[idx]) + "\",";
            data += "\"total_score\":" + std::to_string((int)scores[i]);
            data += "}";
        }
        data += "]";
        return utils::jsonResponse(true, "ok", data);
    }

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

        std::map<std::string, std::vector<double>> classScores;
        for (size_t i = 0; i < students.size(); i++) {
            std::string cls = students[i][3];
            classScores[cls] = std::vector<double>();
        }

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

        std::string data = "[";
        bool first = true;
        for (std::map<std::string, std::vector<double>>::iterator it = classScores.begin();
             it != classScores.end(); ++it) {
            std::vector<double>& scores = it->second;
            if (scores.empty()) continue;
            if (!first) data += ","; first = false;

            double sum = 0;
            int excellent = 0, good = 0, passNum = 0;
            for (size_t i = 0; i < scores.size(); i++) {
                sum += scores[i];
                if (scores[i] >= 90) excellent++;
                if (scores[i] >= 80) good++;
                if (scores[i] >= 60) passNum++;
            }

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

        int ranges[5] = {0, 0, 0, 0, 0};
        const char* labels[5] = {"<60", "60-69", "70-79", "80-89", "90-100"};

        for (size_t i = 0; i < grades.size(); i++) {
            if (grades[i][2] != examId) continue;
            Grade g = Grade::fromCSV(grades[i]);
            double score;
            if (!subject.empty()) {
                std::vector<std::string> slist = exam.getSubjectList();
                score = 0;
                for (size_t j = 0; j < slist.size(); j++) {
                    if (slist[j] == subject) { score = g.getScoreByIndex((int)j); break; }
                }
                double full = exam.getSubjectFull(subject);
                score = score / full * 100;
            } else {
                score = g.calcTotal(exam.getWeightConfig());
            }
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

            // 找考试
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

    std::string getWarnings() {
        std::vector<std::vector<std::string>> grades = storage::getGrades();
        std::vector<std::vector<std::string>> exams = storage::getExams();
        std::vector<std::vector<std::string>> students = storage::getStudents();

        std::string data = "[";
        bool first = true;
        for (size_t i = 0; i < grades.size(); i++) {
            Grade g = Grade::fromCSV(grades[i]);
            if (g.getIsMakeup()) continue;

            std::string eid = grades[i][2];
            Exam exam;
            for (size_t j = 0; j < exams.size(); j++) {
                if (exams[j][0] == eid) { exam = Exam::fromCSV(exams[j]); break; }
            }

            std::vector<std::string> slist = exam.getSubjectList();
            std::vector<double> scoreList = g.getScoreList();

            std::string sname, sclass;
            for (size_t j = 0; j < students.size(); j++) {
                if (students[j][0] == grades[i][1]) {
                    sname = students[j][1];
                    sclass = students[j][3];
                    break;
                }
            }

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

    std::string getEnrollmentStats() {
        std::vector<std::vector<std::string>> students = storage::getStudents();
        std::vector<std::vector<std::string>> classes = storage::getClasses();
        std::map<std::string, int> classCount;
        for (size_t i = 0; i < students.size(); i++) {
            classCount[students[i][3]]++;
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
