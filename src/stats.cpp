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
        return utils::jsonResponse(true, "ok", "[]");
    }

    std::string getTrend(const std::string& studentId) {
        return utils::jsonResponse(true, "ok", "[]");
    }

    std::string getWarnings() {
        return utils::jsonResponse(true, "ok", "[]");
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
