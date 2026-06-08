/*
 * EduGrade 数据读写部分（第 2 部分）
 *
 * 这一部分专门负责：
 * 1. 把 txt 文本记录解析成结构体
 * 2. 把结构体重新写回 txt 文件
 * 3. 首次运行时生成示例数据
 *
 * 由于课程要求使用文件持久化，这里全部基于 fstream 完成。
 */

/* ---------- 文本数据解析与读写 ---------- */

std::map<std::string, std::string> parseScoreMap(const std::string& text) {
    std::map<std::string, std::string> scores;
    if (trim(text).empty()) return scores;

    std::vector<std::string> parts = split(text, ',');
    for (std::size_t i = 0; i < parts.size(); ++i) {
        std::size_t pos = parts[i].find(':');
        if (pos == std::string::npos) continue;
        std::string subject = trim(parts[i].substr(0, pos));
        std::string score = trim(parts[i].substr(pos + 1));
        if (!subject.empty()) {
            scores[subject] = score;
        }
    }
    return scores;
}

std::string scoreMapToText(const std::map<std::string, std::string>& scores) {
    std::vector<std::string> items;
    std::map<std::string, std::string>::const_iterator it = scores.begin();
    for (; it != scores.end(); ++it) {
        items.push_back(it->first + ":" + it->second);
    }
    return join(items, ",");
}

std::vector<User> loadUsers() {
    std::vector<User> users;
    std::vector<std::string> lines = readLines(USERS_FILE);
    for (std::size_t i = 0; i < lines.size(); ++i) {
        std::vector<std::string> parts = split(lines[i], '|');
        if (parts.size() < 5) continue;
        User user;
        user.id = parts[0];
        user.name = parts[1];
        user.role = parts[2];
        user.password = parts[3];
        user.created_at = parts[4];
        users.push_back(user);
    }
    return users;
}

void saveUsers(const std::vector<User>& users) {
    std::vector<std::string> lines;
    for (std::size_t i = 0; i < users.size(); ++i) {
        lines.push_back(users[i].id + "|" + users[i].name + "|" + users[i].role + "|" +
                        users[i].password + "|" + users[i].created_at);
    }
    writeLines(USERS_FILE, lines);
}

std::vector<Student> loadStudents() {
    std::vector<Student> students;
    std::vector<std::string> lines = readLines(STUDENTS_FILE);
    for (std::size_t i = 0; i < lines.size(); ++i) {
        std::vector<std::string> parts = split(lines[i], '|');
        if (parts.size() < 6) continue;

        Student student;
        student.id = parts[0];
        student.name = parts[1];
        student.gender = parts[2];
        student.birthday = parts[3];
        student.class_name = parts[4];
        if (parts.size() >= 7) {
            student.phone = parts[5];
            student.created_at = parts[6];
        } else {
            student.phone = "";
            student.created_at = parts[5];
        }
        students.push_back(student);
    }
    return students;
}

void saveStudents(const std::vector<Student>& students) {
    std::vector<std::string> lines;
    for (std::size_t i = 0; i < students.size(); ++i) {
        lines.push_back(students[i].id + "|" + students[i].name + "|" + students[i].gender + "|" +
                        students[i].birthday + "|" + students[i].class_name + "|" +
                        students[i].phone + "|" + students[i].created_at);
    }
    writeLines(STUDENTS_FILE, lines);
}

std::vector<Subject> loadSubjects() {
    std::vector<Subject> subjects;
    std::vector<std::string> lines = readLines(SUBJECTS_FILE);
    for (std::size_t i = 0; i < lines.size(); ++i) {
        std::vector<std::string> parts = split(lines[i], '|');
        if (parts.size() < 3) continue;
        Subject subject;
        subject.id = parts[0];
        subject.name = parts[1];
        subject.full_score = static_cast<int>(parseDouble(parts[2]));
        if (subject.full_score <= 0) subject.full_score = 100;
        subjects.push_back(subject);
    }
    return subjects;
}

void saveSubjects(const std::vector<Subject>& subjects) {
    std::vector<std::string> lines;
    for (std::size_t i = 0; i < subjects.size(); ++i) {
        lines.push_back(subjects[i].id + "|" + subjects[i].name + "|" +
                        formatDouble(static_cast<double>(subjects[i].full_score)));
    }
    writeLines(SUBJECTS_FILE, lines);
}

std::vector<Exam> loadExams() {
    std::vector<Exam> exams;
    std::vector<std::string> lines = readLines(EXAMS_FILE);
    for (std::size_t i = 0; i < lines.size(); ++i) {
        std::vector<std::string> parts = split(lines[i], '|');
        if (parts.size() < 4) continue;
        Exam exam;
        exam.id = parts[0];
        exam.name = parts[1];
        exam.date = parts[2];
        exam.subjects = split(parts[3], ',');
        for (std::size_t j = 0; j < exam.subjects.size(); ++j) {
            exam.subjects[j] = trim(exam.subjects[j]);
        }
        exams.push_back(exam);
    }
    return exams;
}

void saveExams(const std::vector<Exam>& exams) {
    std::vector<std::string> lines;
    for (std::size_t i = 0; i < exams.size(); ++i) {
        lines.push_back(exams[i].id + "|" + exams[i].name + "|" + exams[i].date + "|" +
                        join(exams[i].subjects, ","));
    }
    writeLines(EXAMS_FILE, lines);
}

std::vector<GradeRecord> loadGrades() {
    std::vector<GradeRecord> grades;
    std::vector<std::string> lines = readLines(GRADES_FILE);
    for (std::size_t i = 0; i < lines.size(); ++i) {
        std::vector<std::string> parts = split(lines[i], '|');
        if (parts.size() < 4) continue;
        GradeRecord grade;
        grade.exam_id = parts[0];
        grade.student_id = parts[1];
        grade.scores = parseScoreMap(parts[2]);
        grade.updated_at = parts[3];
        grades.push_back(grade);
    }
    return grades;
}

void saveGrades(const std::vector<GradeRecord>& grades) {
    std::vector<std::string> lines;
    for (std::size_t i = 0; i < grades.size(); ++i) {
        lines.push_back(grades[i].exam_id + "|" + grades[i].student_id + "|" +
                        scoreMapToText(grades[i].scores) + "|" + grades[i].updated_at);
    }
    writeLines(GRADES_FILE, lines);
}

std::vector<LogEntry> loadLogs() {
    std::vector<LogEntry> logs;
    std::vector<std::string> lines = readLines(LOGS_FILE);
    for (std::size_t i = 0; i < lines.size(); ++i) {
        std::vector<std::string> parts = split(lines[i], '|');
        if (parts.size() < 5) continue;
        LogEntry entry;
        entry.id = parts[0];
        entry.user_id = parts[1];
        entry.action = parts[2];
        entry.detail = parts[3];
        entry.timestamp = parts[4];
        logs.push_back(entry);
    }
    return logs;
}

void saveLogs(const std::vector<LogEntry>& logs) {
    std::vector<std::string> lines;
    for (std::size_t i = 0; i < logs.size(); ++i) {
        lines.push_back(logs[i].id + "|" + logs[i].user_id + "|" + logs[i].action + "|" +
                        logs[i].detail + "|" + logs[i].timestamp);
    }
    writeLines(LOGS_FILE, lines);
}

/* ---------- 初始化示例数据 ---------- */

void writeSeedIfMissing(const std::string& path, const std::vector<std::string>& lines) {
    if (pathExists(path) && !readLines(path).empty()) return;
    writeLines(path, lines);
}

void ensureSeedData() {
    ensureDataDirectory();

    writeSeedIfMissing(USERS_FILE, std::vector<std::string>{
        "admin|系统管理员|admin|123456|2026-02-20 09:00:00",
        "S2024001|张三|student|123456|2026-02-20 09:00:00",
        "S2024002|李四|student|123456|2026-02-20 09:00:00",
        "S2024003|王明|student|123456|2026-02-20 09:00:00",
        "S2024004|陈静|student|123456|2026-02-20 09:00:00",
        "S2024005|刘洋|student|123456|2026-02-20 09:00:00",
        "S2024006|赵敏|student|123456|2026-02-20 09:00:00",
        "S2024007|周航|student|123456|2026-02-20 09:00:00",
        "S2024008|吴倩|student|123456|2026-02-20 09:00:00",
        "S2024009|孙浩|student|123456|2026-02-20 09:00:00",
        "S2024010|何雨|student|123456|2026-02-20 09:00:00"
    });

    writeSeedIfMissing(STUDENTS_FILE, std::vector<std::string>{
        "S2024001|张三|男|2008-01-12|高一1班||2026-02-20 09:00:00",
        "S2024002|李四|女|2008-02-15|高一1班||2026-02-20 09:00:00",
        "S2024003|王明|男|2008-03-08|高一1班||2026-02-20 09:00:00",
        "S2024004|陈静|女|2008-04-19|高一1班||2026-02-20 09:00:00",
        "S2024005|刘洋|男|2008-05-22|高一1班||2026-02-20 09:00:00",
        "S2024006|赵敏|女|2008-01-25|高一2班||2026-02-20 09:00:00",
        "S2024007|周航|男|2008-02-14|高一2班||2026-02-20 09:00:00",
        "S2024008|吴倩|女|2008-03-27|高一2班||2026-02-20 09:00:00",
        "S2024009|孙浩|男|2008-04-09|高一2班||2026-02-20 09:00:00",
        "S2024010|何雨|女|2008-05-31|高一2班||2026-02-20 09:00:00"
    });

    writeSeedIfMissing(SUBJECTS_FILE, std::vector<std::string>{
        "SUB1|语文|150",
        "SUB2|数学|150",
        "SUB3|英语|150"
    });

    writeSeedIfMissing(EXAMS_FILE, std::vector<std::string>{
        "EXAM1|3月月考|2026-03-20|语文,数学,英语",
        "EXAM2|期中考试|2026-05-08|语文,数学,英语"
    });

    writeSeedIfMissing(GRADES_FILE, std::vector<std::string>{
        "EXAM1|S2024001|数学:95,英语:100,语文:98|2026-03-21 18:00:00",
        "EXAM2|S2024001|数学:100,英语:104,语文:102|2026-05-09 18:00:00",
        "EXAM1|S2024002|数学:100,英语:103,语文:102|2026-03-21 18:00:00",
        "EXAM2|S2024002|数学:105,英语:107,语文:106|2026-05-09 18:00:00",
        "EXAM1|S2024003|数学:105,英语:106,语文:106|2026-03-21 18:00:00",
        "EXAM2|S2024003|数学:110,英语:110,语文:110|2026-05-09 18:00:00",
        "EXAM1|S2024004|数学:110,英语:109,语文:110|2026-03-21 18:00:00",
        "EXAM2|S2024004|数学:115,英语:113,语文:114|2026-05-09 18:00:00",
        "EXAM1|S2024005|数学:115,英语:112,语文:114|2026-03-21 18:00:00",
        "EXAM2|S2024005|数学:120,英语:116,语文:118|2026-05-09 18:00:00",
        "EXAM1|S2024006|数学:120,英语:115,语文:118|2026-03-21 18:00:00",
        "EXAM2|S2024006|数学:125,英语:119,语文:122|2026-05-09 18:00:00",
        "EXAM1|S2024007|数学:125,英语:118,语文:122|2026-03-21 18:00:00",
        "EXAM2|S2024007|数学:130,英语:122,语文:126|2026-05-09 18:00:00",
        "EXAM1|S2024008|数学:130,英语:121,语文:126|2026-03-21 18:00:00",
        "EXAM2|S2024008|数学:135,英语:125,语文:130|2026-05-09 18:00:00",
        "EXAM1|S2024009|数学:135,英语:124,语文:130|2026-03-21 18:00:00",
        "EXAM2|S2024009|数学:140,英语:128,语文:134|2026-05-09 18:00:00",
        "EXAM1|S2024010|数学:140,英语:127,语文:134|2026-03-21 18:00:00",
        "EXAM2|S2024010|数学:145,英语:131,语文:138|2026-05-09 18:00:00"
    });

    writeSeedIfMissing(LOGS_FILE, std::vector<std::string>{
        "LOG1|system|初始化|创建示例数据|2026-02-20 09:00:00"
    });
}

