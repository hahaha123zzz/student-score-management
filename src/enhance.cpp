#include "enhance.h"
#include "util.h"
#include <map>
#include <algorithm>
#include <vector>
#include <set>

namespace enhance {

// ===================== Learning Goals =====================
json setGoal(const json& data) {
    std::string studentId = data.value("student_id", "");
    std::string subject = data.value("subject", "");
    if (studentId.empty() || subject.empty())
        return util::errorResponse("学生ID和科目不能为空");

    json goals = util::readJSON("goals.json");
    json entry;
    entry["id"] = "GOAL_" + std::to_string(goals.size() + 1) + "_" + util::getCurrentTime();
    entry["student_id"] = studentId;
    entry["subject"] = subject;
    entry["target_score"] = data.value("target_score", 0);
    entry["exam_name"] = data.value("exam_name", "");
    entry["deadline"] = data.value("deadline", "");
    entry["progress"] = 0;
    entry["created_at"] = util::getCurrentTime();
    goals.push_back(entry);
    util::writeJSON("goals.json", goals);
    return util::jsonResponse(true, "学习目标设置成功", entry);
}

json getGoals(const std::string& studentId) {
    if (studentId.empty())
        return util::errorResponse("学生ID不能为空");

    json goals = util::readJSON("goals.json");
    json grades = util::readJSON("grades.json");
    json exams = util::readJSON("exams.json");

    std::map<std::string, json> examMap;
    for (auto& e : exams) examMap[e["id"]] = e;

    json filtered = json::array();
    for (auto& goal : goals) {
        if (goal.value("student_id", "") != studentId) continue;

        std::string examName = goal.value("exam_name", "");
        std::string subject = goal.value("subject", "");
        double target = goal.value("target_score", 0);
        double achieved = 0;
        bool found = false;

        for (auto& g : grades) {
            if (g["student_id"] != studentId) continue;
            std::string eid = g["exam_id"];
            if (!examMap.count(eid)) continue;
            if (examMap[eid].value("name", "") == examName) {
                if (g["scores"].contains(subject)) {
                    try {
                        achieved = std::stod(g["scores"][subject].get<std::string>());
                        found = true;
                    } catch (...) {}
                }
                break;
            }
        }

        double progress = 0;
        if (found && target > 0)
            progress = std::min(100.0, util::round2(achieved / target * 100));
        goal["progress"] = progress;
        goal["achieved_score"] = found ? achieved : 0;
        filtered.push_back(goal);
    }
    return util::jsonResponse(true, "ok", filtered);
}

// ===================== Knowledge Map =====================
json getKnowledgeMap(const std::string& studentId) {
    if (studentId.empty())
        return util::errorResponse("学生ID不能为空");

    json allErrors = util::readJSON("errors.json");
    json grades = util::readJSON("grades.json");

    // Build master knowledge point list per subject from ALL errors
    std::map<std::string, std::set<std::string>> masterKPs;
    for (auto& e : allErrors) {
        std::string subj = e.value("subject", "");
        std::string kp = e.value("knowledge_point", "");
        if (!subj.empty() && !kp.empty())
            masterKPs[subj].insert(kp);
    }

    // Count target student's errors per (subject, knowledge_point)
    std::map<std::string, std::map<std::string, int>> kpErrors;
    for (auto& e : allErrors) {
        if (e.value("student_id", "") != studentId) continue;
        std::string subj = e.value("subject", "");
        std::string kp = e.value("knowledge_point", "");
        if (!kp.empty())
            kpErrors[subj][kp]++;
    }

    // Collect subjects the student has grades in
    std::set<std::string> gradedSubjects;
    for (auto& g : grades) {
        if (g["student_id"] != studentId) continue;
        for (auto& [subj, _] : g["scores"].items())
            gradedSubjects.insert(subj);
    }

    json result = json::array();

    for (auto& [subj, kps] : masterKPs) {
        json subjectEntry;
        subjectEntry["subject"] = subj;
        json knowledgePoints = json::array();

        for (auto& kp : kps) {
            int count = 0;
            if (kpErrors.count(subj) && kpErrors[subj].count(kp))
                count = kpErrors[subj][kp];

            double mastery;
            if (count > 0)
                mastery = std::max(10.0, 100.0 - count * 15.0);
            else
                mastery = 80.0;

            std::string level;
            if (mastery > 90) level = "mastered";
            else if (mastery >= 70) level = "proficient";
            else if (mastery >= 50) level = "developing";
            else level = "needs_work";

            json kpEntry;
            kpEntry["name"] = kp;
            kpEntry["mastery"] = util::round2(mastery);
            kpEntry["level"] = level;
            kpEntry["error_count"] = count;
            knowledgePoints.push_back(kpEntry);
        }

        std::sort(knowledgePoints.begin(), knowledgePoints.end(), [](const json& a, const json& b) {
            return a["mastery"].get<double>() < b["mastery"].get<double>();
        });

        subjectEntry["knowledge_points"] = knowledgePoints;
        result.push_back(subjectEntry);
    }

    // Subjects with grades but no KP definitions
    for (auto& subj : gradedSubjects) {
        if (masterKPs.count(subj)) continue;
        json subjectEntry;
        subjectEntry["subject"] = subj;
        json knowledgePoints = json::array();
        json kpEntry;
        kpEntry["name"] = subj + "综合";
        kpEntry["mastery"] = 80;
        kpEntry["level"] = "proficient";
        kpEntry["error_count"] = 0;
        knowledgePoints.push_back(kpEntry);
        subjectEntry["knowledge_points"] = knowledgePoints;
        result.push_back(subjectEntry);
    }

    return util::jsonResponse(true, "ok", result);
}

// ===================== Growth Portfolio =====================
json getPortfolio(const std::string& studentId) {
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

    struct GradeEntry {
        std::string examId;
        std::string examName;
        std::string date;
        double overall;
        json details;
    };
    std::vector<GradeEntry> gradeEntries;
    std::map<std::string, std::vector<double>> subjectScores;

    for (auto& g : grades) {
        if (g["student_id"] != studentId) continue;
        std::string eid = g["exam_id"];
        json eData = examMap.count(eid) ? examMap[eid] : json::object();
        json wc = eData.value("weight_config", json::object());

        GradeEntry entry;
        entry.examId = eid;
        entry.examName = eData.value("name", eid);
        entry.date = eData.value("date", "");

        double sum = 0;
        int cnt = 0;
        entry.details = json::array();
        for (auto& [subj, scoreStr] : g["scores"].items()) {
            double score = 0;
            try { score = std::stod(scoreStr.get<std::string>()); } catch (...) {}
            double full = 100;
            if (wc.contains(subj)) full = wc[subj].value("full", 100.0);
            double norm = (full > 0) ? (score / full * 100) : 0;

            json d;
            d["subject"] = subj;
            d["score"] = score;
            d["full_score"] = full;
            d["normalized"] = util::round2(norm);
            entry.details.push_back(d);

            subjectScores[subj].push_back(norm);
            sum += norm;
            cnt++;
        }
        entry.overall = cnt > 0 ? util::round2(sum / cnt) : 0;
        gradeEntries.push_back(entry);
    }

    std::sort(gradeEntries.begin(), gradeEntries.end(), [](const GradeEntry& a, const GradeEntry& b) {
        return a.date < b.date;
    });

    int totalErrorCount = 0;
    for (auto& e : errors)
        if (e.value("student_id", "") == studentId) totalErrorCount++;

    json trend = json::array();
    for (auto& ge : gradeEntries) {
        json t;
        t["exam_name"] = ge.examName;
        t["date"] = ge.date;
        t["overall"] = ge.overall;
        t["details"] = ge.details;
        trend.push_back(t);
    }

    double maxOverall = 0, minOverall = 100, sumOverall = 0;
    for (auto& ge : gradeEntries) {
        if (ge.overall > maxOverall) maxOverall = ge.overall;
        if (ge.overall < minOverall) minOverall = ge.overall;
        sumOverall += ge.overall;
    }
    double avgOverall = gradeEntries.empty() ? 0 : util::round2(sumOverall / gradeEntries.size());
    if (gradeEntries.empty()) { maxOverall = 0; minOverall = 0; }

    double improvement = 0;
    if (gradeEntries.size() >= 2)
        improvement = util::round2(gradeEntries.back().overall - gradeEntries.front().overall);

    std::string strongestSubject, weakestSubject;
    double bestAvg = -1, worstAvg = 101;
    for (auto& [subj, scores] : subjectScores) {
        double sum = 0;
        for (double s : scores) sum += s;
        double avg = util::round2(sum / scores.size());
        if (avg > bestAvg) { bestAvg = avg; strongestSubject = subj; }
        if (avg < worstAvg) { worstAvg = avg; weakestSubject = subj; }
    }

    double participationRate = exams.empty() ? 0
        : util::round2((double)gradeEntries.size() / exams.size() * 100);

    json result;
    result["student"] = studentInfo;
    result["grades"] = trend;
    result["total_exams_taken"] = (int)gradeEntries.size();
    result["max_overall"] = maxOverall;
    result["min_overall"] = minOverall;
    result["average_overall"] = avgOverall;
    result["improvement_index"] = improvement;
    result["total_errors"] = totalErrorCount;
    result["strongest_subject"] = strongestSubject;
    result["strongest_subject_avg"] = bestAvg;
    result["weakest_subject"] = weakestSubject;
    result["weakest_subject_avg"] = worstAvg;
    result["exam_participation_rate"] = participationRate;

    return util::jsonResponse(true, "ok", result);
}

// ===================== Points System =====================
json getPoints(const std::string& studentId) {
    if (studentId.empty())
        return util::errorResponse("学生ID不能为空");

    json pointsData = util::readJSON("points.json");
    for (auto& p : pointsData) {
        if (p.value("student_id", "") == studentId)
            return util::jsonResponse(true, "ok", p);
    }

    json newRecord;
    newRecord["student_id"] = studentId;
    newRecord["points"] = 0;
    newRecord["history"] = json::array();
    return util::jsonResponse(true, "ok", newRecord);
}

json addPoints(const std::string& studentId, int points, const std::string& reason) {
    if (studentId.empty())
        return util::errorResponse("学生ID不能为空");

    json pointsData = util::readJSON("points.json");
    int idx = -1;
    for (int i = 0; i < (int)pointsData.size(); i++) {
        if (pointsData[i].value("student_id", "") == studentId) {
            idx = i; break;
        }
    }

    json record;
    if (idx >= 0)
        record = pointsData[idx];
    else {
        record["student_id"] = studentId;
        record["points"] = 0;
        record["history"] = json::array();
    }

    record["points"] = record["points"].get<int>() + points;

    json historyEntry;
    historyEntry["reason"] = reason;
    historyEntry["points_added"] = points;
    historyEntry["timestamp"] = util::getCurrentTime();
    record["history"].push_back(historyEntry);

    if (idx >= 0)
        pointsData[idx] = record;
    else
        pointsData.push_back(record);

    util::writeJSON("points.json", pointsData);
    return util::jsonResponse(true, "积分添加成功", record);
}

// ===================== Review Plan =====================
json getReviewPlan(const std::string& studentId) {
    if (studentId.empty())
        return util::errorResponse("学生ID不能为空");

    json errors = util::readJSON("errors.json");

    struct GroupKey {
        std::string kp;
        std::string etype;
        bool operator<(const GroupKey& other) const {
            if (kp != other.kp) return kp < other.kp;
            return etype < other.etype;
        }
    };

    std::map<GroupKey, std::vector<json>> groups;
    for (auto& e : errors) {
        if (e.value("student_id", "") != studentId) continue;
        GroupKey key{e.value("knowledge_point", ""), e.value("error_type", "未分类")};
        groups[key].push_back(e);
    }

    std::vector<std::pair<GroupKey, std::vector<json>>> sortedGroups;
    for (auto& [key, errs] : groups)
        sortedGroups.push_back({key, errs});
    std::sort(sortedGroups.begin(), sortedGroups.end(),
        [](const auto& a, const auto& b) { return a.second.size() > b.second.size(); });

    json plan = json::array();
    std::vector<int> intervals = {1, 2, 4, 7, 15, 30};

    for (auto& [key, errs] : sortedGroups) {
        std::string subject = errs[0].value("subject", "");
        std::string firstDate = errs[0].value("created_at", "");

        json item;
        item["knowledge_point"] = key.kp;
        item["error_type"] = key.etype;
        item["subject"] = subject;
        item["error_count"] = (int)errs.size();
        item["first_error_date"] = firstDate;

        json reviewSchedule = json::array();
        for (int day : intervals) {
            json interval;
            interval["day"] = day;
            interval["description"] = "第" + std::to_string(day) + "天复习";
            reviewSchedule.push_back(interval);
        }
        item["review_intervals"] = reviewSchedule;

        json questions = json::array();
        for (auto& e : errs) {
            json q;
            q["question"] = e.value("question", "");
            q["correct_answer"] = e.value("correct_answer", "");
            questions.push_back(q);
        }
        item["questions"] = questions;

        plan.push_back(item);
    }

    json result;
    result["plan"] = plan;
    result["total_errors"] = (int)errors.size();
    return util::jsonResponse(true, "ok", result);
}

// ===================== Mental Check-in =====================
json checkin(const json& data) {
    std::string studentId = data.value("student_id", "");
    if (studentId.empty())
        return util::errorResponse("学生ID不能为空");

    int mood = data.value("mood", 0);
    if (mood < 1 || mood > 5)
        return util::errorResponse("心情值范围1-5");

    json checkins = util::readJSON("checkins.json");
    json entry;
    entry["id"] = "CK_" + studentId + "_" + util::getCurrentTime();
    entry["student_id"] = studentId;
    entry["mood"] = mood;
    entry["note"] = data.value("note", "");
    entry["timestamp"] = data.value("timestamp", util::getCurrentTime());
    checkins.push_back(entry);
    util::writeJSON("checkins.json", checkins);

    util::logAction(studentId, "心情打卡", "心情值: " + std::to_string(mood));
    return util::jsonResponse(true, "打卡成功", entry);
}

json getCheckins(const std::string& studentId) {
    if (studentId.empty())
        return util::errorResponse("学生ID不能为空");

    json checkins = util::readJSON("checkins.json");
    json filtered = json::array();
    for (auto& c : checkins) {
        if (c.value("student_id", "") == studentId)
            filtered.push_back(c);
    }
    return util::jsonResponse(true, "ok", filtered);
}

}
