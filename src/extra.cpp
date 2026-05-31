#include "extra.h"
#include "util.h"
#include <map>
#include <algorithm>
#include <cmath>
#include <set>
#include <sstream>

namespace extra {

json setError(const json& data) {
    std::string studentId = data.value("student_id", "");
    std::string examId = data.value("exam_id", "");
    std::string subject = data.value("subject", "");
    std::string question = data.value("question", "");
    if (studentId.empty() || examId.empty() || subject.empty())
        return util::errorResponse("学生ID、考试ID和科目不能为空");

    json errors = util::readJSON("errors.json");
    json entry;
    entry["id"] = "ERR_" + std::to_string(errors.size() + 1) + "_" + util::getCurrentTime();
    entry["student_id"] = studentId;
    entry["exam_id"] = examId;
    entry["subject"] = subject;
    entry["question"] = question;
    entry["wrong_answer"] = data.value("wrong_answer", "");
    entry["correct_answer"] = data.value("correct_answer", "");
    entry["error_type"] = data.value("error_type", "未分类");
    entry["knowledge_point"] = data.value("knowledge_point", "");
    entry["created_at"] = util::getCurrentTime();
    errors.push_back(entry);
    util::writeJSON("errors.json", errors);
    return util::jsonResponse(true, "错题记录成功", entry);
}

json getErrors(const std::string& studentId, const std::string& examId, const std::string& subject, int page, int size) {
    json errors = util::readJSON("errors.json");
    json filtered = json::array();
    for (auto& e : errors) {
        if (!studentId.empty() && e.value("student_id", "") != studentId) continue;
        if (!examId.empty() && e.value("exam_id", "") != examId) continue;
        if (!subject.empty() && e.value("subject", "") != subject) continue;
        filtered.push_back(e);
    }
    std::reverse(filtered.begin(), filtered.end());
    int total = (int)filtered.size();
    if (page < 1) page = 1;
    if (size < 1) size = 20;
    int start = (page - 1) * size;
    int end = std::min(start + size, total);
    json result;
    result["total"] = total;
    result["page"] = page;
    result["size"] = size;
    result["data"] = json::array();
    for (int i = start; i < end; i++)
        result["data"].push_back(filtered[i]);
    return util::jsonResponse(true, "ok", result);
}

json getErrorStats(const std::string& studentId) {
    if (studentId.empty())
        return util::errorResponse("学生ID不能为空");

    json errors = util::readJSON("errors.json");
    std::map<std::string, int> typeCount;
    std::map<std::string, int> subjectCount;
    std::map<std::string, std::map<std::string, int>> knowledgeCount;
    int total = 0;

    for (auto& e : errors) {
        if (e.value("student_id", "") != studentId) continue;
        total++;
        std::string etype = e.value("error_type", "未分类");
        std::string esubj = e.value("subject", "");
        std::string ekp = e.value("knowledge_point", "");
        typeCount[etype]++;
        subjectCount[esubj]++;
        if (!ekp.empty()) knowledgeCount[esubj][ekp]++;
    }

    json byType = json::array();
    for (auto& [t, c] : typeCount) {
        json item;
        item["error_type"] = t;
        item["count"] = c;
        byType.push_back(item);
    }
    std::sort(byType.begin(), byType.end(), [](const json& a, const json& b) {
        return a["count"].get<int>() > b["count"].get<int>();
    });

    json bySubject = json::array();
    for (auto& [s, c] : subjectCount) {
        json item;
        item["subject"] = s;
        item["count"] = c;
        bySubject.push_back(item);
    }
    std::sort(bySubject.begin(), bySubject.end(), [](const json& a, const json& b) {
        return a["count"].get<int>() > b["count"].get<int>();
    });

    json weakPoints = json::array();
    for (auto& [subj, kps] : knowledgeCount) {
        for (auto& [kp, cnt] : kps) {
            if (cnt >= 2) {
                json wp;
                wp["subject"] = subj;
                wp["knowledge_point"] = kp;
                wp["count"] = cnt;
                weakPoints.push_back(wp);
            }
        }
    }
    std::sort(weakPoints.begin(), weakPoints.end(), [](const json& a, const json& b) {
        return a["count"].get<int>() > b["count"].get<int>();
    });

    json result;
    result["by_type"] = byType;
    result["by_subject"] = bySubject;
    result["total_count"] = total;
    result["weak_points"] = weakPoints;
    return util::jsonResponse(true, "ok", result);
}

json getImbalance(const std::string& studentId) {
    if (studentId.empty())
        return util::errorResponse("学生ID不能为空");

    json grades = util::readJSON("grades.json");
    json exams = util::readJSON("exams.json");

    std::map<std::string, json> examMap;
    for (auto& e : exams) examMap[e["id"]] = e;

    std::map<std::string, std::vector<double>> subjectScores;
    std::set<std::string> allSubjects;

    for (auto& g : grades) {
        if (g["student_id"] != studentId) continue;
        std::string eid = g["exam_id"];
        if (!examMap.count(eid)) continue;
        json wc = examMap[eid].value("weight_config", json::object());

        for (auto& [subj, scoreStr] : g["scores"].items()) {
            allSubjects.insert(subj);
            double score = 0;
            try { score = std::stod(scoreStr.get<std::string>()); } catch (...) {}
            double full = 100;
            if (wc.contains(subj)) full = wc[subj].value("full", 100.0);
            double normalized = (full > 0) ? (score / full * 100) : 0;
            subjectScores[subj].push_back(normalized);
        }
    }

    json imbalanceData = json::array();
    double overallMaxCv = 0;
    int subjectCount = 0;

    for (auto& subj : allSubjects) {
        auto& scores = subjectScores[subj];
        if (scores.empty()) continue;
        subjectCount++;
        double sum = 0, maxVal = 0;
        for (double s : scores) { sum += s; if (s > maxVal) maxVal = s; }
        double avg = sum / scores.size();
        double variance = 0;
        for (double s : scores) variance += (s - avg) * (s - avg);
        double stddev = std::sqrt(variance / scores.size());
        double cv = (avg > 0) ? (stddev / avg * 100) : 0;

        json item;
        item["subject"] = subj;
        item["max"] = util::round2(maxVal);
        item["average"] = util::round2(avg);
        item["stddev"] = util::round2(stddev);
        item["cv"] = util::round2(cv);
        item["exam_count"] = (int)scores.size();

        std::string level;
        if (cv < 5) level = "均衡";
        else if (cv < 15) level = "轻微偏科";
        else if (cv < 25) level = "中度偏科";
        else level = "严重偏科";
        item["imbalance_index"] = cv;
        item["imbalance_level"] = level;
        imbalanceData.push_back(item);
        if (cv > overallMaxCv) overallMaxCv = cv;
    }

    std::sort(imbalanceData.begin(), imbalanceData.end(), [](const json& a, const json& b) {
        return a["cv"].get<double>() > b["cv"].get<double>();
    });

    std::string overallLevel;
    if (overallMaxCv < 10) overallLevel = "各科发展均衡，继续保持";
    else if (overallMaxCv < 20) overallLevel = "存在轻微偏科现象，建议适当加强弱势科目";
    else if (overallMaxCv < 30) overallLevel = "存在明显偏科，需要重点关注弱势科目学习";
    else overallLevel = "偏科严重，建议制定专项提升计划";

    json result;
    result["imbalance_data"] = imbalanceData;
    result["overall_level"] = overallLevel;
    result["overall_max_cv"] = util::round2(overallMaxCv);
    result["suggestion"] = overallLevel;
    return util::jsonResponse(true, "ok", result);
}

json getAbilityMap(const std::string& studentId, const std::string& examId) {
    if (studentId.empty() || examId.empty())
        return util::errorResponse("学生ID和考试ID不能为空");

    json grades = util::readJSON("grades.json");
    json exams = util::readJSON("exams.json");

    json examData;
    for (auto& e : exams)
        if (e["id"] == examId) { examData = e; break; }
    if (examData.is_null()) return util::errorResponse("考试不存在");

    json wc = examData.value("weight_config", json::object());

    json gradeEntry;
    bool found = false;
    for (auto& g : grades) {
        if (g["student_id"] == studentId && g["exam_id"] == examId) {
            gradeEntry = g; found = true; break;
        }
    }
    if (!found) return util::errorResponse("未找到该学生的成绩记录");

    json subjects = json::array();
    double sumNorm = 0, maxNorm = 0;
    int count = 0;

    for (auto& subj : examData["subjects"]) {
        std::string sname = subj.get<std::string>();
        json item;
        item["name"] = sname;
        if (gradeEntry["scores"].contains(sname)) {
            double score = 0;
            try { score = std::stod(gradeEntry["scores"][sname].get<std::string>()); } catch (...) {}
            double full = 100;
            if (wc.contains(sname)) full = wc[sname].value("full", 100.0);
            double normalized = (full > 0) ? (score / full * 100) : 0;
            item["score"] = score;
            item["normalized"] = util::round2(normalized);
            auto r = util::gradeToRank(gradeEntry["scores"][sname].get<std::string>(), full);
            item["level"] = r["grade"];
            sumNorm += normalized;
            if (normalized > maxNorm) maxNorm = normalized;
            count++;
        } else {
            item["score"] = 0;
            item["normalized"] = 0;
            item["level"] = "N/A";
        }
        subjects.push_back(item);
    }

    json result;
    result["subjects"] = subjects;
    result["max_value"] = util::round2(maxNorm);
    result["avg_value"] = count > 0 ? util::round2(sumNorm / count) : 0;
    return util::jsonResponse(true, "ok", result);
}

json getReport(const std::string& studentId) {
    if (studentId.empty())
        return util::errorResponse("学生ID不能为空");

    json students = util::readJSON("students.json");
    json grades = util::readJSON("grades.json");
    json exams = util::readJSON("exams.json");
    json errors = util::readJSON("errors.json");

    json studentInfo;
    for (auto& s : students) {
        if (s["id"] == studentId) { studentInfo = s; break; }
    }
    if (studentInfo.is_null()) return util::errorResponse("学生不存在");

    std::map<std::string, json> examMap;
    for (auto& e : exams) examMap[e["id"]] = e;

    json allGrades = json::array();
    json subjectTotals;
    json subjectCounts;
    double totalAll = 0;
    int gradeCount = 0;

    for (auto& g : grades) {
        if (g["student_id"] != studentId) continue;
        std::string eid = g["exam_id"];
        json eData = examMap.count(eid) ? examMap[eid] : json::object();
        json wc = eData.value("weight_config", json::object());

        json gradeItem;
        gradeItem["exam_id"] = eid;
        gradeItem["exam_name"] = eData.value("name", eid);
        gradeItem["date"] = eData.value("date", "");
        gradeItem["is_makeup"] = g.value("is_makeup", false);
        json details = json::array();

        for (auto& [subj, scoreStr] : g["scores"].items()) {
            double score = 0;
            try { score = std::stod(scoreStr.get<std::string>()); } catch (...) {}
            double full = 100;
            if (wc.contains(subj)) full = wc[subj].value("full", 100.0);
            double norm = (full > 0) ? util::round2(score / full * 100) : 0;
            json d;
            d["subject"] = subj;
            d["score"] = score;
            d["full"] = full;
            d["normalized"] = norm;
            details.push_back(d);
            double prevTotal = (subjectTotals.contains(subj) && subjectTotals[subj].is_number()) ? subjectTotals[subj].get<double>() : 0.0;
            subjectTotals[subj] = prevTotal + norm;
            int prevCnt = (subjectCounts.contains(subj) && subjectCounts[subj].is_number()) ? subjectCounts[subj].get<int>() : 0;
            subjectCounts[subj] = prevCnt + 1;
        }
        gradeItem["details"] = details;
        allGrades.push_back(gradeItem);
        gradeCount++;
        totalAll += 0;
    }

    for (auto& g : allGrades) {
        double sum = 0;
        int cnt = 0;
        for (auto& d : g["details"]) { sum += d["normalized"].get<double>(); cnt++; }
        g["overall"] = cnt > 0 ? util::round2(sum / cnt) : 0;
    }

    std::sort(allGrades.begin(), allGrades.end(), [](const json& a, const json& b) {
        return a["date"].get<std::string>() < b["date"].get<std::string>();
    });

    json subjectAverages = json::array();
    for (auto& [subj, total] : subjectTotals.items()) {
        int cnt = (subjectCounts.contains(subj) && subjectCounts[subj].is_number()) ? subjectCounts[subj].get<int>() : 0;
        json item;
        item["subject"] = subj;
        item["average"] = cnt > 0 ? util::round2(total.get<double>() / cnt) : 0;
        item["count"] = cnt;
        subjectAverages.push_back(item);
    }

    std::sort(subjectAverages.begin(), subjectAverages.end(), [](const json& a, const json& b) {
        return a["average"].get<double>() < b["average"].get<double>();
    });

    json weaknesses = json::array();
    for (auto& sa : subjectAverages) {
        if (sa["average"].get<double>() < 70 && sa["average"].get<double>() > 0) {
            weaknesses.push_back(sa["subject"]);
        }
    }

    json trend = json::array();
    for (auto& g : allGrades) {
        json t;
        t["exam_name"] = g["exam_name"];
        t["date"] = g["date"];
        t["overall"] = g["overall"];
        trend.push_back(t);
    }

    int errorTotal = 0;
    for (auto& e : errors)
        if (e.value("student_id", "") == studentId) errorTotal++;

    json result;
    result["student"] = studentInfo;
    result["grades"] = allGrades;
    result["subject_averages"] = subjectAverages;
    result["weaknesses"] = weaknesses;
    result["trend"] = trend;
    result["total_exams"] = (int)allGrades.size();
    result["total_errors"] = errorTotal;
    return util::jsonResponse(true, "ok", result);
}

json getSuggest(const std::string& studentId) {
    if (studentId.empty())
        return util::errorResponse("学生ID不能为空");

    json grades = util::readJSON("grades.json");
    json exams = util::readJSON("exams.json");
    json errors = util::readJSON("errors.json");

    std::map<std::string, json> examMap;
    for (auto& e : exams) examMap[e["id"]] = e;

    std::map<std::string, std::vector<double>> subjectScores;
    for (auto& g : grades) {
        if (g["student_id"] != studentId) continue;
        std::string eid = g["exam_id"];
        json wc = examMap.count(eid) ? examMap[eid].value("weight_config", json::object()) : json::object();
        for (auto& [subj, scoreStr] : g["scores"].items()) {
            double score = 0;
            try { score = std::stod(scoreStr.get<std::string>()); } catch (...) {}
            double full = 100;
            if (wc.contains(subj)) full = wc[subj].value("full", 100.0);
            double norm = (full > 0) ? (score / full * 100) : 0;
            subjectScores[subj].push_back(norm);
        }
    }

    std::map<std::string, double> subjectAvgs;
    for (auto& [subj, scores] : subjectScores) {
        if (scores.empty()) continue;
        double sum = 0;
        for (double s : scores) sum += s;
        subjectAvgs[subj] = sum / scores.size();
    }

    std::map<std::string, int> errorTypeCount;
    std::map<std::string, int> errorSubjectCount;
    for (auto& e : errors) {
        if (e.value("student_id", "") != studentId) continue;
        errorTypeCount[e.value("error_type", "未分类")]++;
        errorSubjectCount[e.value("subject", "")]++;
    }

    json suggestions = json::array();

    std::vector<std::pair<std::string, double>> sortedSubjs;
    for (auto& [s, a] : subjectAvgs) sortedSubjs.push_back({s, a});
    std::sort(sortedSubjs.begin(), sortedSubjs.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; });

    int priority = 1;
    for (auto& [subj, avg] : sortedSubjs) {
        if (avg >= 85) continue;
        int errCount = 0;
        for (auto& [es, ec] : errorSubjectCount)
            if (es == subj) errCount = ec;

        json s;
        s["subject"] = subj;
        s["priority"] = priority;
        std::string text;
        if (avg < 60)
            text = subj + "成绩处于不及格水平(" + std::to_string((int)avg) + "分)，建议从基础知识点重新梳理，每天安排额外1小时专项练习，寻求老师或同学帮助";
        else if (avg < 75)
            text = subj + "成绩偏弱(" + std::to_string((int)avg) + "分)，建议针对薄弱知识点进行针对性训练，整理错题本并定期回顾";
        else
            text = subj + "成绩良好(" + std::to_string((int)avg) + "分)，建议保持当前学习状态，适当挑战难题以进一步提升";
        if (errCount > 0)
            text += "，已有" + std::to_string(errCount) + "道错题记录，建议逐一分析错误原因";
        s["suggestion_text"] = text;
        suggestions.push_back(s);
        priority++;
    }

    if (suggestions.empty()) {
        json s;
        s["subject"] = "综合";
        s["priority"] = 1;
        s["suggestion_text"] = "各科表现均良好，建议保持当前学习状态，适当拓展课外知识，培养综合能力";
        suggestions.push_back(s);
    }

    return util::jsonResponse(true, "ok", suggestions);
}

}
