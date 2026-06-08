/*
 * EduGrade 业务支撑部分（第 3 部分）
 *
 * 这一部分负责把“原始数据”变成“可用业务能力”：
 * 1. 各种查找函数
 * 2. 日志写入
 * 3. 登录态与权限校验
 * 4. 总分、平均分、排名等核心计算
 * 5. 把结构体转换成前端能直接使用的 JSON 字符串
 */

/* ---------- 查找辅助函数 ---------- */

int findUserIndex(const std::vector<User>& users, const std::string& id) {
    for (std::size_t i = 0; i < users.size(); ++i) {
        if (users[i].id == id) return static_cast<int>(i);
    }
    return -1;
}

int findStudentIndex(const std::vector<Student>& students, const std::string& id) {
    for (std::size_t i = 0; i < students.size(); ++i) {
        if (students[i].id == id) return static_cast<int>(i);
    }
    return -1;
}

int findSubjectIndex(const std::vector<Subject>& subjects, const std::string& id) {
    for (std::size_t i = 0; i < subjects.size(); ++i) {
        if (subjects[i].id == id) return static_cast<int>(i);
    }
    return -1;
}

int findSubjectIndexByName(const std::vector<Subject>& subjects, const std::string& name) {
    for (std::size_t i = 0; i < subjects.size(); ++i) {
        if (subjects[i].name == name) return static_cast<int>(i);
    }
    return -1;
}

int findExamIndex(const std::vector<Exam>& exams, const std::string& id) {
    for (std::size_t i = 0; i < exams.size(); ++i) {
        if (exams[i].id == id) return static_cast<int>(i);
    }
    return -1;
}

int findGradeIndex(const std::vector<GradeRecord>& grades, const std::string& exam_id, const std::string& student_id) {
    for (std::size_t i = 0; i < grades.size(); ++i) {
        if (grades[i].exam_id == exam_id && grades[i].student_id == student_id) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

std::string nextIdNumber(const std::string& prefix, const std::vector<std::string>& ids) {
    int max_value = 0;
    for (std::size_t i = 0; i < ids.size(); ++i) {
        if (ids[i].find(prefix) == 0) {
            std::string suffix = ids[i].substr(prefix.size());
            int value = static_cast<int>(parseDouble(suffix));
            if (value > max_value) max_value = value;
        }
    }
    return prefix + formatDouble(static_cast<double>(max_value + 1));
}

std::string nextSubjectId(const std::vector<Subject>& subjects) {
    std::vector<std::string> ids;
    for (std::size_t i = 0; i < subjects.size(); ++i) ids.push_back(subjects[i].id);
    return nextIdNumber("SUB", ids);
}

std::string nextExamId(const std::vector<Exam>& exams) {
    std::vector<std::string> ids;
    for (std::size_t i = 0; i < exams.size(); ++i) ids.push_back(exams[i].id);
    return nextIdNumber("EXAM", ids);
}

std::string nextLogId(const std::vector<LogEntry>& logs) {
    std::vector<std::string> ids;
    for (std::size_t i = 0; i < logs.size(); ++i) ids.push_back(logs[i].id);
    return nextIdNumber("LOG", ids);
}

std::string scoreLevel(double score, double full_score) {
    double percent = full_score <= 0 ? 0 : (score / full_score) * 100.0;
    if (percent >= 90) return "优秀";
    if (percent >= 80) return "良好";
    if (percent >= 70) return "中等";
    if (percent >= 60) return "及格";
    return "不及格";
}

bool scoreIsAbsent(const std::string& score_text) {
    std::string text = trim(score_text);
    return text.empty() || text == "ABS" || text == "缺考";
}

int fullScoreForSubject(const std::vector<Subject>& subjects, const std::string& subject_name) {
    int index = findSubjectIndexByName(subjects, subject_name);
    if (index < 0) return 100;
    return subjects[static_cast<std::size_t>(index)].full_score;
}

/* ---------- 日志写入辅助函数 ---------- */

void addLog(const std::string& user_id, const std::string& action, const std::string& detail) {
    std::vector<LogEntry> logs = loadLogs();
    LogEntry entry;
    entry.id = nextLogId(logs);
    entry.user_id = user_id.empty() ? "system" : user_id;
    entry.action = action;
    entry.detail = detail;
    entry.timestamp = currentTimeText();
    logs.push_back(entry);
    saveLogs(logs);
}

/* ---------- 登录态与权限校验 ---------- */

std::string extractToken(const Request& req) {
    std::string header = req.get_header_value("Authorization");
    if (header.find("Bearer ") == 0) {
        return header.substr(7);
    }
    return "";
}

bool requireAuth(const Request& req, Response& res, SessionInfo& session) {
    std::string token = extractToken(req);
    std::map<std::string, SessionInfo>::iterator it = g_sessions.find(token);
    if (token.empty() || it == g_sessions.end()) {
        setError(res, 401, "请先登录");
        return false;
    }
    session = it->second;
    return true;
}

bool requireAdmin(const Request& req, Response& res, SessionInfo& session) {
    if (!requireAuth(req, res, session)) return false;
    if (session.role != "admin") {
        setError(res, 403, "只有管理员可以执行此操作");
        return false;
    }
    return true;
}

bool requireSelfOrAdmin(const Request& req, Response& res, const std::string& target_id, SessionInfo& session) {
    if (!requireAuth(req, res, session)) return false;
    if (session.role == "admin") return true;
    if (session.user_id != target_id) {
        setError(res, 403, "只能访问自己的数据");
        return false;
    }
    return true;
}

/* ---------- 成绩计算与排名逻辑 ---------- */

double calculateTotal(const GradeRecord& grade, const Exam& exam) {
    double total = 0.0;
    for (std::size_t i = 0; i < exam.subjects.size(); ++i) {
        std::map<std::string, std::string>::const_iterator it = grade.scores.find(exam.subjects[i]);
        if (it != grade.scores.end() && !scoreIsAbsent(it->second)) {
            total += parseDouble(it->second);
        }
    }
    return total;
}

double calculateAverage(const GradeRecord& grade, const Exam& exam) {
    if (exam.subjects.empty()) return 0.0;
    return calculateTotal(grade, exam) / static_cast<double>(exam.subjects.size());
}

std::vector<RankItem> buildRankItems(const std::string& exam_id,
                                     const std::string& sort_mode,
                                     const std::string& subject_name,
                                     const std::vector<Student>& students,
                                     const std::vector<Exam>& exams,
                                     const std::vector<GradeRecord>& grades) {
    std::vector<RankItem> items;
    int exam_index = findExamIndex(exams, exam_id);
    if (exam_index < 0) return items;

    const Exam& exam = exams[static_cast<std::size_t>(exam_index)];
    for (std::size_t i = 0; i < grades.size(); ++i) {
        if (grades[i].exam_id != exam_id) continue;
        int student_index = findStudentIndex(students, grades[i].student_id);
        if (student_index < 0) continue;

        RankItem item;
        item.student = students[static_cast<std::size_t>(student_index)];
        item.grade = grades[i];
        item.total = calculateTotal(grades[i], exam);
        item.average = calculateAverage(grades[i], exam);
        item.class_rank = 0;
        item.grade_rank = 0;
        items.push_back(item);
    }

    std::sort(items.begin(), items.end(), [&](const RankItem& left, const RankItem& right) {
        double left_key = left.total;
        double right_key = right.total;
        if (sort_mode == "subject" && !subject_name.empty()) {
            std::map<std::string, std::string>::const_iterator left_it = left.grade.scores.find(subject_name);
            std::map<std::string, std::string>::const_iterator right_it = right.grade.scores.find(subject_name);
            left_key = (left_it == left.grade.scores.end() || scoreIsAbsent(left_it->second)) ? 0.0 : parseDouble(left_it->second);
            right_key = (right_it == right.grade.scores.end() || scoreIsAbsent(right_it->second)) ? 0.0 : parseDouble(right_it->second);
        }
        if (left_key != right_key) return left_key > right_key;
        return left.student.id < right.student.id;
    });

    std::map<std::string, int> class_counter;
    for (std::size_t i = 0; i < items.size(); ++i) {
        items[i].grade_rank = static_cast<int>(i) + 1;
        class_counter[items[i].student.class_name] += 1;
        items[i].class_rank = class_counter[items[i].student.class_name];
    }

    return items;
}

std::vector<std::string> uniqueClasses(const std::vector<Student>& students) {
    std::set<std::string> class_set;
    std::vector<std::string> classes;
    for (std::size_t i = 0; i < students.size(); ++i) {
        if (class_set.insert(students[i].class_name).second) {
            classes.push_back(students[i].class_name);
        }
    }
    std::sort(classes.begin(), classes.end());
    return classes;
}

std::vector<std::string> uniqueGrades(const std::vector<Student>& students) {
    std::set<std::string> grade_set;
    std::vector<std::string> grades;
    for (std::size_t i = 0; i < students.size(); ++i) {
        std::string grade = students[i].gradeName();
        if (grade_set.insert(grade).second) {
            grades.push_back(grade);
        }
    }
    std::sort(grades.begin(), grades.end());
    return grades;
}

/* ---------- 数据转 JSON 字符串 ---------- */

std::string serializeStringArray(const std::vector<std::string>& values) {
    std::vector<std::string> items;
    for (std::size_t i = 0; i < values.size(); ++i) {
        items.push_back(jsonString(values[i]));
    }
    return makeArray(items);
}

std::string serializeStudent(const Student& student) {
    std::vector<std::string> fields;
    fields.push_back(fieldString("id", student.id));
    fields.push_back(fieldString("name", student.name));
    fields.push_back(fieldString("gender", student.gender));
    fields.push_back(fieldString("birthday", student.birthday));
    fields.push_back(fieldString("class_name", student.class_name));
    fields.push_back(fieldString("grade_name", student.gradeName()));
    fields.push_back(fieldString("phone", student.phone));
    fields.push_back(fieldString("created_at", student.created_at));
    return makeObject(fields);
}

std::string serializeSubject(const Subject& subject) {
    std::vector<std::string> fields;
    fields.push_back(fieldString("id", subject.id));
    fields.push_back(fieldString("name", subject.name));
    fields.push_back(fieldInteger("full_score", subject.full_score));
    return makeObject(fields);
}

std::string serializeExam(const Exam& exam) {
    std::vector<std::string> fields;
    fields.push_back(fieldString("id", exam.id));
    fields.push_back(fieldString("name", exam.name));
    fields.push_back(fieldString("date", exam.date));
    fields.push_back(fieldRaw("subjects", serializeStringArray(exam.subjects)));
    return makeObject(fields);
}

std::string serializeScoreMap(const std::map<std::string, std::string>& scores, const std::vector<std::string>& order) {
    std::vector<std::string> fields;
    for (std::size_t i = 0; i < order.size(); ++i) {
        std::map<std::string, std::string>::const_iterator it = scores.find(order[i]);
        std::string value = it == scores.end() ? "" : it->second;
        fields.push_back(fieldString(order[i], value));
    }
    return makeObject(fields);
}

std::string serializeScoreList(const GradeRecord& grade, const Exam& exam, const std::vector<Subject>& subjects) {
    std::vector<std::string> items;
    for (std::size_t i = 0; i < exam.subjects.size(); ++i) {
        const std::string& subject_name = exam.subjects[i];
        std::map<std::string, std::string>::const_iterator score_it = grade.scores.find(subject_name);
        std::string score_text = score_it == grade.scores.end() ? "" : score_it->second;
        bool absent = scoreIsAbsent(score_text);
        double score_value = absent ? 0.0 : parseDouble(score_text);
        int full_score = fullScoreForSubject(subjects, subject_name);

        std::vector<std::string> fields;
        fields.push_back(fieldString("subject", subject_name));
        fields.push_back(fieldString("score_text", absent ? "缺考" : score_text));
        fields.push_back(fieldNumber("score", score_value));
        fields.push_back(fieldInteger("full_score", full_score));
        fields.push_back(fieldBool("absent", absent));
        fields.push_back(fieldString("level", scoreLevel(score_value, static_cast<double>(full_score))));
        items.push_back(makeObject(fields));
    }
    return makeArray(items);
}

std::string serializeGradeRecord(const GradeRecord& grade,
                                 const Student& student,
                                 const Exam& exam,
                                 const std::vector<Subject>& subjects,
                                 int class_rank,
                                 int grade_rank) {
    std::vector<std::string> fields;
    fields.push_back(fieldString("exam_id", grade.exam_id));
    fields.push_back(fieldString("exam_name", exam.name));
    fields.push_back(fieldString("exam_date", exam.date));
    fields.push_back(fieldString("student_id", grade.student_id));
    fields.push_back(fieldString("student_name", student.name));
    fields.push_back(fieldString("class_name", student.class_name));
    fields.push_back(fieldString("grade_name", student.gradeName()));
    fields.push_back(fieldRaw("score_map", serializeScoreMap(grade.scores, exam.subjects)));
    fields.push_back(fieldRaw("scores", serializeScoreList(grade, exam, subjects)));
    fields.push_back(fieldNumber("total", calculateTotal(grade, exam)));
    fields.push_back(fieldNumber("average", calculateAverage(grade, exam)));
    fields.push_back(fieldInteger("class_rank", class_rank));
    fields.push_back(fieldInteger("grade_rank", grade_rank));
    fields.push_back(fieldString("updated_at", grade.updated_at));
    return makeObject(fields);
}

std::string serializeRankItem(const RankItem& item, const Exam& exam) {
    std::vector<std::string> fields;
    fields.push_back(fieldString("student_id", item.student.id));
    fields.push_back(fieldString("student_name", item.student.name));
    fields.push_back(fieldString("class_name", item.student.class_name));
    fields.push_back(fieldString("grade_name", item.student.gradeName()));
    fields.push_back(fieldRaw("score_map", serializeScoreMap(item.grade.scores, exam.subjects)));
    fields.push_back(fieldNumber("total", item.total));
    fields.push_back(fieldNumber("average", item.average));
    fields.push_back(fieldInteger("class_rank", item.class_rank));
    fields.push_back(fieldInteger("grade_rank", item.grade_rank));
    return makeObject(fields);
}

std::string serializeLog(const LogEntry& log) {
    std::vector<std::string> fields;
    fields.push_back(fieldString("id", log.id));
    fields.push_back(fieldString("user_id", log.user_id));
    fields.push_back(fieldString("action", log.action));
    fields.push_back(fieldString("detail", log.detail));
    fields.push_back(fieldString("timestamp", log.timestamp));
    return makeObject(fields);
}

/* ---------- 概览数据与统计结果拼装 ---------- */

std::string buildDashboardJson() {
    std::vector<Student> students = loadStudents();
    std::vector<Subject> subjects = loadSubjects();
    std::vector<Exam> exams = loadExams();
    std::vector<GradeRecord> grades = loadGrades();
    std::vector<LogEntry> logs = loadLogs();

    std::string latest_exam_id = "";
    if (!exams.empty()) {
        latest_exam_id = exams[0].id;
        for (std::size_t i = 1; i < exams.size(); ++i) {
            int current_index = findExamIndex(exams, latest_exam_id);
            if (current_index >= 0 && exams[i].date > exams[static_cast<std::size_t>(current_index)].date) {
                latest_exam_id = exams[i].id;
            }
        }
    }

    std::vector<std::string> top_students_json;
    std::string latest_exam_json = "null";
    if (!latest_exam_id.empty()) {
        int exam_index = findExamIndex(exams, latest_exam_id);
        std::vector<RankItem> items = buildRankItems(latest_exam_id, "total", "", students, exams, grades);
        for (std::size_t i = 0; i < items.size() && i < 5; ++i) {
            top_students_json.push_back(serializeRankItem(items[i], exams[static_cast<std::size_t>(exam_index)]));
        }
        latest_exam_json = serializeExam(exams[static_cast<std::size_t>(exam_index)]);
    }

    std::vector<std::string> recent_logs_json;
    std::size_t start_index = logs.size() > 5 ? logs.size() - 5 : 0;
    for (std::size_t i = start_index; i < logs.size(); ++i) {
        recent_logs_json.push_back(serializeLog(logs[i]));
    }

    std::vector<std::string> fields;
    fields.push_back(fieldInteger("student_count", static_cast<int>(students.size())));
    fields.push_back(fieldInteger("class_count", static_cast<int>(uniqueClasses(students).size())));
    fields.push_back(fieldInteger("subject_count", static_cast<int>(subjects.size())));
    fields.push_back(fieldInteger("exam_count", static_cast<int>(exams.size())));
    fields.push_back(fieldInteger("grade_count", static_cast<int>(grades.size())));
    fields.push_back(fieldRaw("latest_exam", latest_exam_json));
    fields.push_back(fieldRaw("top_students", makeArray(top_students_json)));
    fields.push_back(fieldRaw("recent_logs", makeArray(recent_logs_json)));
    return makeObject(fields);
}

std::string buildClassListJson() {
    std::vector<Student> students = loadStudents();
    std::vector<std::string> classes = uniqueClasses(students);
    std::vector<std::string> items;
    for (std::size_t i = 0; i < classes.size(); ++i) {
        int count = 0;
        std::string grade = "未知年级";
        for (std::size_t j = 0; j < students.size(); ++j) {
            if (students[j].class_name == classes[i]) {
                ++count;
                grade = students[j].gradeName();
            }
        }
        std::vector<std::string> fields;
        fields.push_back(fieldString("class_name", classes[i]));
        fields.push_back(fieldString("grade_name", grade));
        fields.push_back(fieldInteger("student_count", count));
        items.push_back(makeObject(fields));
    }
    std::vector<std::string> root_fields;
    root_fields.push_back(fieldRaw("items", makeArray(items)));
    root_fields.push_back(fieldRaw("grades", serializeStringArray(uniqueGrades(students))));
    return makeObject(root_fields);
}

