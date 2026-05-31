#include "stats.h"
#include "util.h"
#include <map>
#include <algorithm>

namespace stats {

json convertGrade(const json& scores, const json& weightConfig) {
    double totalWeighted = 0, totalWeight = 0;
    json details = json::array();
    for (auto& [subj, scoreStr] : scores.items()) {
        double score = 0;
        try { score = std::stod(scoreStr.get<std::string>()); } catch (...) {}
        double full = 100, weight = 1.0;
        if (weightConfig.contains(subj)) {
            full = weightConfig[subj].value("full", 100.0);
            weight = weightConfig[subj].value("weight", 1.0);
        }
        totalWeighted += (score / full) * weight;
        totalWeight += weight;
        auto r = util::gradeToRank(scoreStr.get<std::string>(), full);
        json d;
        d["subject"] = subj;
        d["score"] = score;
        d["full"] = full;
        d["grade"] = r["grade"];
        d["gpa"] = r["gpa"];
        details.push_back(d);
    }
    double totalScore = totalWeight > 0 ? (totalWeighted / totalWeight) * 100 : 0;
    json result;
    result["total_score"] = util::round2(totalScore);
    result["details"] = details;
    return result;
}

json getRank(const std::string& examId, const std::string& type, const std::string& subject) {
    json grades = util::readJSON("grades.json");
    json students = util::readJSON("students.json");
    json exams = util::readJSON("exams.json");

    json examData;
    for (auto& e : exams) if (e["id"] == examId) { examData = e; break; }
    json wc = examData.value("weight_config", json::object());

    std::vector<json> rankings;
    for (auto& g : grades) {
        if (g["exam_id"] != examId) continue;
        json rank;
        rank["student_id"] = g["student_id"];
        for (auto& s : students) {
            if (s["id"] == g["student_id"]) {
                rank["student_name"] = s["name"];
                rank["class"] = s["class"];
                break;
            }
        }

        if (!subject.empty()) {
            double score = 0;
            if (g["scores"].contains(subject)) {
                try { score = std::stod(g["scores"][subject].get<std::string>()); } catch (...) {}
            }
            rank["total_score"] = score;
            rank["subject"] = subject;
        } else {
            json converted = convertGrade(g["scores"], wc);
            rank["total_score"] = converted["total_score"];
            rank["details"] = converted["details"];
        }
        rank["is_makeup"] = g.value("is_makeup", false);
        rankings.push_back(rank);
    }

    std::sort(rankings.begin(), rankings.end(), [](const json& a, const json& b) {
        return a["total_score"].get<double>() > b["total_score"].get<double>();
    });

    int rank = 1;
    for (auto& r : rankings) r["rank"] = rank++;

    if (type == "class") {
        std::map<std::string, std::vector<json>> classMap;
        for (auto& r : rankings)
            classMap[r["class"]].push_back(r);
        json result = json::object();
        for (auto& [cls, vec] : classMap) {
            for (size_t i = 0; i < vec.size(); i++)
                vec[i]["class_rank"] = (int)i + 1;
            result[cls] = vec;
        }
        return util::jsonResponse(true, "ok", result);
    }

    return util::jsonResponse(true, "ok", rankings);
}

json getClassCompare(const std::string& examId) {
    json grades = util::readJSON("grades.json");
    json students = util::readJSON("students.json");
    json exams = util::readJSON("exams.json");

    json examData;
    for (auto& e : exams) if (e["id"] == examId) { examData = e; break; }
    json wc = examData.value("weight_config", json::object());

    std::map<std::string, std::vector<double>> classScores;
    for (auto& st : students) {
        classScores[st["class"].get<std::string>()] = {};
    }

    for (auto& g : grades) {
        if (g["exam_id"] != examId) continue;
        std::string cls;
        for (auto& st : students) {
            if (st["id"] == g["student_id"]) { cls = st["class"]; break; }
        }
        if (cls.empty()) continue;
        json converted = convertGrade(g["scores"], wc);
        classScores[cls].push_back(converted["total_score"].get<double>());
    }

    json result = json::array();
    for (auto& [cls, scores] : classScores) {
        if (scores.empty()) continue;
        double sum = 0;
        int excellent = 0, good = 0, pass = 0;
        for (double s : scores) {
            sum += s;
            if (s >= 90) excellent++;
            if (s >= 80) good++;
            if (s >= 60) pass++;
        }
        int n = scores.size();
        json item;
        item["class"] = cls;
        item["count"] = n;
        item["average"] = util::round2(sum / n);
        item["excellent_rate"] = util::round2((double)excellent / n * 100);
        item["good_rate"] = util::round2((double)good / n * 100);
        item["pass_rate"] = util::round2((double)pass / n * 100);
        std::sort(scores.begin(), scores.end());
        item["max"] = scores.back();
        item["min"] = scores.front();
        item["median"] = scores[n / 2];
        result.push_back(item);
    }

    std::sort(result.begin(), result.end(), [](const json& a, const json& b) {
        return a["average"].get<double>() > b["average"].get<double>();
    });

    return util::jsonResponse(true, "ok", result);
}

json getDistribution(const std::string& examId, const std::string& subject) {
    json grades = util::readJSON("grades.json");
    json exams = util::readJSON("exams.json");

    json examData;
    for (auto& e : exams) if (e["id"] == examId) { examData = e; break; }
    json wc = examData.value("weight_config", json::object());

    int ranges[5] = {0, 0, 0, 0, 0}; // <60, 60-69, 70-79, 80-89, 90-100
    const char* labels[5] = {"<60", "60-69", "70-79", "80-89", "90-100"};

    for (auto& g : grades) {
        if (g["exam_id"] != examId) continue;
        double score = 0;
        if (!subject.empty()) {
            if (g["scores"].contains(subject))
                try { score = std::stod(g["scores"][subject].get<std::string>()); } catch (...) {}
            double full = 100;
            if (wc.contains(subject)) full = wc[subject].value("full", 100.0);
            score = score / full * 100;
        } else {
            json converted = convertGrade(g["scores"], wc);
            score = converted["total_score"].get<double>();
        }
        if (score < 60) ranges[0]++;
        else if (score < 70) ranges[1]++;
        else if (score < 80) ranges[2]++;
        else if (score < 90) ranges[3]++;
        else ranges[4]++;
    }

    json result = json::array();
    for (int i = 0; i < 5; i++) {
        json item;
        item["range"] = labels[i];
        item["count"] = ranges[i];
        result.push_back(item);
    }
    return util::jsonResponse(true, "ok", result);
}

json getTrend(const std::string& studentId) {
    json grades = util::readJSON("grades.json");
    json exams = util::readJSON("exams.json");

    std::map<std::string, json> examMap;
    for (auto& e : exams) examMap[e["id"]] = e;

    json trends = json::array();
    for (auto& g : grades) {
        if (g["student_id"] != studentId) continue;
        std::string eid = g["exam_id"];
        json wc = examMap[eid].value("weight_config", json::object());
        json converted = convertGrade(g["scores"], wc);
        json point;
        point["exam_id"] = eid;
        point["exam_name"] = examMap[eid].value("name", eid);
        point["date"] = examMap[eid].value("date", "");
        point["total_score"] = converted["total_score"];
        point["details"] = converted["details"];
        point["is_makeup"] = g.value("is_makeup", false);
        trends.push_back(point);
    }
    return util::jsonResponse(true, "ok", trends);
}

json getWarnings() {
    json grades = util::readJSON("grades.json");
    json exams = util::readJSON("exams.json");
    json students = util::readJSON("students.json");

    std::map<std::string, json> examMap, studentMap;
    for (auto& e : exams) examMap[e["id"]] = e;
    for (auto& s : students) studentMap[s["id"]] = s;

    json warnings = json::array();
    for (auto& g : grades) {
        std::string sid = g["student_id"];
        if (g.value("is_makeup", false)) continue;
        json wc = examMap[g["exam_id"]].value("weight_config", json::object());
        json converted = convertGrade(g["scores"], wc);
        double total = converted["total_score"].get<double>();

        for (auto& [subj, scoreStr] : g["scores"].items()) {
            double score = 0;
            try { score = std::stod(scoreStr.get<std::string>()); } catch (...) {}
            double full = 100;
            if (wc.contains(subj)) full = wc[subj].value("full", 100.0);
            if (score / full * 100 < 60) {
                json w;
                w["student_id"] = sid;
                w["student_name"] = studentMap[sid].value("name", sid);
                w["class"] = studentMap[sid].value("class", "");
                w["exam_name"] = examMap[g["exam_id"]].value("name", "");
                w["subject"] = subj;
                w["score"] = score;
                w["full"] = full;
                w["type"] = "不及格";
                warnings.push_back(w);
            }
        }
    }
    return util::jsonResponse(true, "ok", warnings);
}

}
