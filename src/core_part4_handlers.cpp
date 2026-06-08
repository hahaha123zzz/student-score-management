/*
 * EduGrade 接口处理部分（第 4 部分）
 *
 * 这一部分直接对应浏览器发来的请求。
 * 每个处理函数基本都遵循同一个步骤：
 * 1. 读取参数
 * 2. 做权限检查
 * 3. 调用前面几部分的业务函数
 * 4. 返回统一格式的 JSON 响应
 */

/* ---------- 接口处理函数 ---------- */

void handleLogin(const Request& req, Response& res) {
    std::map<std::string, std::string> form = parseFormBody(req.body);
    std::string username = trim(formValue(form, "username"));
    std::string password = formValue(form, "password");

    if (username.empty() || password.empty()) {
        setError(res, 400, "用户名和密码不能为空");
        return;
    }

    std::vector<User> users = loadUsers();
    int user_index = findUserIndex(users, username);
    if (user_index < 0) {
        setError(res, 404, "用户不存在");
        return;
    }

    User user = users[static_cast<std::size_t>(user_index)];
    if (user.password != password) {
        setError(res, 403, "密码错误");
        return;
    }

    std::string token = randomToken();
    SessionInfo session;
    session.user_id = user.id;
    session.name = user.name;
    session.role = user.role;
    g_sessions[token] = session;

    addLog(user.id, "登录", user.name + " 登录系统");

    std::vector<std::string> data_fields;
    data_fields.push_back(fieldString("token", token));
    data_fields.push_back(fieldString("user_id", user.id));
    data_fields.push_back(fieldString("name", user.name));
    data_fields.push_back(fieldString("role", user.role));
    setJson(res, makeResponse(true, "登录成功", makeObject(data_fields)));
}

void handleAuthCheck(const Request& req, Response& res) {
    SessionInfo session;
    if (!requireAuth(req, res, session)) return;
    std::vector<std::string> fields;
    fields.push_back(fieldString("user_id", session.user_id));
    fields.push_back(fieldString("name", session.name));
    fields.push_back(fieldString("role", session.role));
    setJson(res, makeResponse(true, "认证有效", makeObject(fields)));
}

void handleChangePassword(const Request& req, Response& res) {
    SessionInfo session;
    if (!requireAuth(req, res, session)) return;

    std::map<std::string, std::string> form = parseFormBody(req.body);
    std::string old_password = formValue(form, "old_password");
    std::string new_password = formValue(form, "new_password");

    if (new_password.size() < 4) {
        setError(res, 400, "新密码至少 4 位");
        return;
    }

    std::vector<User> users = loadUsers();
    int user_index = findUserIndex(users, session.user_id);
    if (user_index < 0) {
        setError(res, 404, "用户不存在");
        return;
    }
    if (users[static_cast<std::size_t>(user_index)].password != old_password) {
        setError(res, 403, "原密码错误");
        return;
    }

    users[static_cast<std::size_t>(user_index)].password = new_password;
    saveUsers(users);
    addLog(session.user_id, "修改密码", "修改登录密码");

    setJson(res, makeResponse(true, "密码修改成功", "null"));
}

void handleListClasses(const Request& req, Response& res) {
    SessionInfo session;
    if (!requireAuth(req, res, session)) return;
    setJson(res, makeResponse(true, "ok", buildClassListJson()));
}

void handleListStudents(const Request& req, Response& res) {
    SessionInfo session;
    if (!requireAdmin(req, res, session)) return;

    std::vector<Student> students = loadStudents();
    std::string search = trim(queryValue(req, "search"));
    std::string class_name = trim(queryValue(req, "class_name"));
    std::string grade_name = trim(queryValue(req, "grade_name"));
    std::string search_lower = toLowerAscii(search);

    std::vector<std::string> items;
    for (std::size_t i = 0; i < students.size(); ++i) {
        bool matches = true;
        if (!search.empty()) {
            std::string id_text = toLowerAscii(students[i].id);
            std::string name_text = toLowerAscii(students[i].name);
            matches = (id_text.find(search_lower) != std::string::npos ||
                       name_text.find(search_lower) != std::string::npos);
        }
        if (matches && !class_name.empty() && students[i].class_name != class_name) {
            matches = false;
        }
        if (matches && !grade_name.empty() && students[i].gradeName() != grade_name) {
            matches = false;
        }
        if (matches) items.push_back(serializeStudent(students[i]));
    }

    std::vector<std::string> fields;
    fields.push_back(fieldRaw("items", makeArray(items)));
    fields.push_back(fieldRaw("classes", serializeStringArray(uniqueClasses(students))));
    fields.push_back(fieldRaw("grades", serializeStringArray(uniqueGrades(students))));
    setJson(res, makeResponse(true, "ok", makeObject(fields)));
}

void handleGetStudent(const Request& req, Response& res, const std::string& student_id) {
    SessionInfo session;
    if (!requireSelfOrAdmin(req, res, student_id, session)) return;

    std::vector<Student> students = loadStudents();
    int student_index = findStudentIndex(students, student_id);
    if (student_index < 0) {
        setError(res, 404, "学生不存在");
        return;
    }

    setJson(res, makeResponse(true, "ok", serializeStudent(students[static_cast<std::size_t>(student_index)])));
}

void handleAddStudent(const Request& req, Response& res) {
    SessionInfo session;
    if (!requireAdmin(req, res, session)) return;

    std::map<std::string, std::string> form = parseFormBody(req.body);
    Student student;
    student.id = trim(formValue(form, "id"));
    student.name = trim(formValue(form, "name"));
    student.gender = trim(formValue(form, "gender"));
    student.birthday = trim(formValue(form, "birthday"));
    student.class_name = trim(formValue(form, "class_name"));
    student.phone = trim(formValue(form, "phone"));
    student.created_at = currentTimeText();

    if (student.id.empty() || student.name.empty() || student.class_name.empty()) {
        setError(res, 400, "学号、姓名和班级不能为空");
        return;
    }

    std::vector<Student> students = loadStudents();
    std::vector<User> users = loadUsers();
    if (findStudentIndex(students, student.id) >= 0 || findUserIndex(users, student.id) >= 0) {
        setError(res, 400, "学号已存在");
        return;
    }

    students.push_back(student);
    saveStudents(students);

    User user;
    user.id = student.id;
    user.name = student.name;
    user.role = "student";
    user.password = "123456";
    user.created_at = currentTimeText();
    users.push_back(user);
    saveUsers(users);

    addLog(session.user_id, "添加学生", "添加学生 " + student.id + " " + student.name);
    setJson(res, makeResponse(true, "学生添加成功", serializeStudent(student)));
}

void handleUpdateStudent(const Request& req, Response& res, const std::string& student_id) {
    SessionInfo session;
    if (!requireSelfOrAdmin(req, res, student_id, session)) return;

    std::map<std::string, std::string> form = parseFormBody(req.body);
    std::vector<Student> students = loadStudents();
    std::vector<User> users = loadUsers();
    int student_index = findStudentIndex(students, student_id);
    if (student_index < 0) {
        setError(res, 404, "学生不存在");
        return;
    }

    Student& student = students[static_cast<std::size_t>(student_index)];
    if (!formValue(form, "name").empty()) student.name = trim(formValue(form, "name"));
    if (!formValue(form, "gender").empty()) student.gender = trim(formValue(form, "gender"));
    if (!formValue(form, "birthday").empty()) student.birthday = trim(formValue(form, "birthday"));
    if (!formValue(form, "phone").empty() || session.role != "admin") student.phone = trim(formValue(form, "phone"));

    if (session.role == "admin" && !formValue(form, "class_name").empty()) {
        student.class_name = trim(formValue(form, "class_name"));
    }

    int user_index = findUserIndex(users, student_id);
    if (user_index >= 0) {
        users[static_cast<std::size_t>(user_index)].name = student.name;
        saveUsers(users);
    }

    saveStudents(students);
    addLog(session.user_id, "更新学生", "更新学生 " + student.id);
    setJson(res, makeResponse(true, "学生信息更新成功", serializeStudent(student)));
}

void handleDeleteStudent(const Request& req, Response& res, const std::string& student_id) {
    SessionInfo session;
    if (!requireAdmin(req, res, session)) return;

    std::vector<Student> students = loadStudents();
    std::vector<User> users = loadUsers();
    std::vector<GradeRecord> grades = loadGrades();
    int student_index = findStudentIndex(students, student_id);
    if (student_index < 0) {
        setError(res, 404, "学生不存在");
        return;
    }

    students.erase(students.begin() + student_index);
    saveStudents(students);

    int user_index = findUserIndex(users, student_id);
    if (user_index >= 0) {
        users.erase(users.begin() + user_index);
        saveUsers(users);
    }

    std::vector<GradeRecord> filtered_grades;
    for (std::size_t i = 0; i < grades.size(); ++i) {
        if (grades[i].student_id != student_id) filtered_grades.push_back(grades[i]);
    }
    saveGrades(filtered_grades);

    addLog(session.user_id, "删除学生", "删除学生 " + student_id);
    setJson(res, makeResponse(true, "学生已删除", "null"));
}

void handleListSubjects(const Request& req, Response& res) {
    SessionInfo session;
    if (!requireAuth(req, res, session)) return;
    std::vector<Subject> subjects = loadSubjects();
    std::vector<std::string> items;
    for (std::size_t i = 0; i < subjects.size(); ++i) {
        items.push_back(serializeSubject(subjects[i]));
    }
    setJson(res, makeResponse(true, "ok", makeObject(std::vector<std::string>{fieldRaw("items", makeArray(items))})));
}

void handleAddSubject(const Request& req, Response& res) {
    SessionInfo session;
    if (!requireAdmin(req, res, session)) return;
    std::map<std::string, std::string> form = parseFormBody(req.body);
    std::string name = trim(formValue(form, "name"));
    int full_score = static_cast<int>(parseDouble(formValue(form, "full_score")));

    if (name.empty() || full_score <= 0) {
        setError(res, 400, "科目名称和满分不能为空");
        return;
    }

    std::vector<Subject> subjects = loadSubjects();
    if (findSubjectIndexByName(subjects, name) >= 0) {
        setError(res, 400, "科目已存在");
        return;
    }

    Subject subject;
    subject.id = nextSubjectId(subjects);
    subject.name = name;
    subject.full_score = full_score;
    subjects.push_back(subject);
    saveSubjects(subjects);

    addLog(session.user_id, "添加科目", "添加科目 " + name);
    setJson(res, makeResponse(true, "科目添加成功", serializeSubject(subject)));
}

void handleUpdateSubject(const Request& req, Response& res, const std::string& subject_id) {
    SessionInfo session;
    if (!requireAdmin(req, res, session)) return;

    std::map<std::string, std::string> form = parseFormBody(req.body);
    std::vector<Subject> subjects = loadSubjects();
    std::vector<Exam> exams = loadExams();
    std::vector<GradeRecord> grades = loadGrades();
    int subject_index = findSubjectIndex(subjects, subject_id);
    if (subject_index < 0) {
        setError(res, 404, "科目不存在");
        return;
    }

    Subject& subject = subjects[static_cast<std::size_t>(subject_index)];
    std::string old_name = subject.name;
    std::string new_name = trim(formValue(form, "name"));
    int full_score = static_cast<int>(parseDouble(formValue(form, "full_score")));

    if (!new_name.empty() && new_name != old_name) {
        if (findSubjectIndexByName(subjects, new_name) >= 0) {
            setError(res, 400, "新的科目名称已存在");
            return;
        }
        subject.name = new_name;

        for (std::size_t i = 0; i < exams.size(); ++i) {
            for (std::size_t j = 0; j < exams[i].subjects.size(); ++j) {
                if (exams[i].subjects[j] == old_name) {
                    exams[i].subjects[j] = new_name;
                }
            }
        }

        for (std::size_t i = 0; i < grades.size(); ++i) {
            std::map<std::string, std::string>::iterator score_it = grades[i].scores.find(old_name);
            if (score_it != grades[i].scores.end()) {
                std::string score_text = score_it->second;
                grades[i].scores.erase(score_it);
                grades[i].scores[new_name] = score_text;
            }
        }
    }

    if (full_score > 0) {
        subject.full_score = full_score;
    }

    saveSubjects(subjects);
    saveExams(exams);
    saveGrades(grades);
    addLog(session.user_id, "更新科目", "更新科目 " + subject_id);
    setJson(res, makeResponse(true, "科目更新成功", serializeSubject(subject)));
}

void handleDeleteSubject(const Request& req, Response& res, const std::string& subject_id) {
    SessionInfo session;
    if (!requireAdmin(req, res, session)) return;

    std::vector<Subject> subjects = loadSubjects();
    std::vector<Exam> exams = loadExams();
    int subject_index = findSubjectIndex(subjects, subject_id);
    if (subject_index < 0) {
        setError(res, 404, "科目不存在");
        return;
    }

    std::string subject_name = subjects[static_cast<std::size_t>(subject_index)].name;
    for (std::size_t i = 0; i < exams.size(); ++i) {
        if (std::find(exams[i].subjects.begin(), exams[i].subjects.end(), subject_name) != exams[i].subjects.end()) {
            setError(res, 400, "该科目已被考试使用，不能删除");
            return;
        }
    }

    subjects.erase(subjects.begin() + subject_index);
    saveSubjects(subjects);
    addLog(session.user_id, "删除科目", "删除科目 " + subject_name);
    setJson(res, makeResponse(true, "科目已删除", "null"));
}

std::vector<std::string> parseSubjectText(const std::string& text) {
    std::vector<std::string> raw_parts = split(text, ',');
    std::vector<std::string> subjects;
    for (std::size_t i = 0; i < raw_parts.size(); ++i) {
        std::string value = trim(raw_parts[i]);
        if (!value.empty()) subjects.push_back(value);
    }
    return subjects;
}

bool subjectsExist(const std::vector<Subject>& subjects, const std::vector<std::string>& names) {
    for (std::size_t i = 0; i < names.size(); ++i) {
        if (findSubjectIndexByName(subjects, names[i]) < 0) {
            return false;
        }
    }
    return true;
}

void handleListExams(const Request& req, Response& res) {
    SessionInfo session;
    if (!requireAuth(req, res, session)) return;

    std::vector<Exam> exams = loadExams();
    std::vector<std::string> items;
    for (std::size_t i = 0; i < exams.size(); ++i) {
        items.push_back(serializeExam(exams[i]));
    }
    setJson(res, makeResponse(true, "ok", makeObject(std::vector<std::string>{fieldRaw("items", makeArray(items))})));
}

void handleAddExam(const Request& req, Response& res) {
    SessionInfo session;
    if (!requireAdmin(req, res, session)) return;

    std::map<std::string, std::string> form = parseFormBody(req.body);
    std::string name = trim(formValue(form, "name"));
    std::string date = trim(formValue(form, "date"));
    std::vector<std::string> selected_subjects = parseSubjectText(formValue(form, "subjects"));

    if (name.empty() || date.empty() || selected_subjects.empty()) {
        setError(res, 400, "考试名称、日期和科目不能为空");
        return;
    }

    std::vector<Subject> subjects = loadSubjects();
    if (!subjectsExist(subjects, selected_subjects)) {
        setError(res, 400, "考试包含不存在的科目");
        return;
    }

    std::vector<Exam> exams = loadExams();
    Exam exam;
    exam.id = nextExamId(exams);
    exam.name = name;
    exam.date = date;
    exam.subjects = selected_subjects;
    exams.push_back(exam);
    saveExams(exams);

    addLog(session.user_id, "添加考试", "添加考试 " + name);
    setJson(res, makeResponse(true, "考试添加成功", serializeExam(exam)));
}

void handleUpdateExam(const Request& req, Response& res, const std::string& exam_id) {
    SessionInfo session;
    if (!requireAdmin(req, res, session)) return;

    std::map<std::string, std::string> form = parseFormBody(req.body);
    std::vector<Exam> exams = loadExams();
    int exam_index = findExamIndex(exams, exam_id);
    if (exam_index < 0) {
        setError(res, 404, "考试不存在");
        return;
    }

    std::vector<Subject> subjects = loadSubjects();
    Exam& exam = exams[static_cast<std::size_t>(exam_index)];
    std::string name = trim(formValue(form, "name"));
    std::string date = trim(formValue(form, "date"));
    std::string subjects_text = formValue(form, "subjects");

    if (!name.empty()) exam.name = name;
    if (!date.empty()) exam.date = date;
    if (!subjects_text.empty()) {
        std::vector<std::string> selected_subjects = parseSubjectText(subjects_text);
        if (selected_subjects.empty() || !subjectsExist(subjects, selected_subjects)) {
            setError(res, 400, "考试科目不合法");
            return;
        }
        exam.subjects = selected_subjects;
    }

    saveExams(exams);
    addLog(session.user_id, "更新考试", "更新考试 " + exam_id);
    setJson(res, makeResponse(true, "考试更新成功", serializeExam(exam)));
}

void handleDeleteExam(const Request& req, Response& res, const std::string& exam_id) {
    SessionInfo session;
    if (!requireAdmin(req, res, session)) return;

    std::vector<Exam> exams = loadExams();
    std::vector<GradeRecord> grades = loadGrades();
    int exam_index = findExamIndex(exams, exam_id);
    if (exam_index < 0) {
        setError(res, 404, "考试不存在");
        return;
    }

    exams.erase(exams.begin() + exam_index);
    saveExams(exams);

    std::vector<GradeRecord> filtered_grades;
    for (std::size_t i = 0; i < grades.size(); ++i) {
        if (grades[i].exam_id != exam_id) filtered_grades.push_back(grades[i]);
    }
    saveGrades(filtered_grades);

    addLog(session.user_id, "删除考试", "删除考试 " + exam_id);
    setJson(res, makeResponse(true, "考试已删除", "null"));
}

std::map<std::string, std::string> parseGradeScores(const std::string& scores_text) {
    std::map<std::string, std::string> scores = parseScoreMap(scores_text);
    std::map<std::string, std::string>::iterator it = scores.begin();
    for (; it != scores.end(); ++it) {
        if (trim(it->second).empty()) it->second = "ABS";
    }
    return scores;
}

std::map<std::string, std::string> normalizeGradeScoresForExam(const Exam& exam,
                                                               const std::map<std::string, std::string>& raw_scores) {
    std::map<std::string, std::string> normalized;
    for (std::size_t i = 0; i < exam.subjects.size(); ++i) {
        const std::string& subject_name = exam.subjects[i];
        std::map<std::string, std::string>::const_iterator it = raw_scores.find(subject_name);
        if (it == raw_scores.end() || trim(it->second).empty()) {
            normalized[subject_name] = "ABS";
        } else {
            normalized[subject_name] = trim(it->second);
        }
    }
    return normalized;
}

void handleListGrades(const Request& req, Response& res) {
    SessionInfo session;
    if (!requireAdmin(req, res, session)) return;

    std::string exam_id = trim(queryValue(req, "exam_id"));
    std::string student_id = trim(queryValue(req, "student_id"));
    std::string class_name = trim(queryValue(req, "class_name"));

    std::vector<Student> students = loadStudents();
    std::vector<Subject> subjects = loadSubjects();
    std::vector<Exam> exams = loadExams();
    std::vector<GradeRecord> grades = loadGrades();

    std::vector<std::string> items;
    for (std::size_t i = 0; i < grades.size(); ++i) {
        if (!exam_id.empty() && grades[i].exam_id != exam_id) continue;
        if (!student_id.empty() && grades[i].student_id != student_id) continue;

        int student_index = findStudentIndex(students, grades[i].student_id);
        int exam_index = findExamIndex(exams, grades[i].exam_id);
        if (student_index < 0 || exam_index < 0) continue;

        const Student& student = students[static_cast<std::size_t>(student_index)];
        if (!class_name.empty() && student.class_name != class_name) continue;

        std::vector<RankItem> rank_items = buildRankItems(grades[i].exam_id, "total", "", students, exams, grades);
        int class_rank = 0;
        int grade_rank = 0;
        for (std::size_t k = 0; k < rank_items.size(); ++k) {
            if (rank_items[k].student.id == grades[i].student_id) {
                class_rank = rank_items[k].class_rank;
                grade_rank = rank_items[k].grade_rank;
                break;
            }
        }

        items.push_back(serializeGradeRecord(grades[i],
                                            student,
                                            exams[static_cast<std::size_t>(exam_index)],
                                            subjects,
                                            class_rank,
                                            grade_rank));
    }

    std::vector<std::string> fields;
    fields.push_back(fieldRaw("items", makeArray(items)));
    setJson(res, makeResponse(true, "ok", makeObject(fields)));
}

void handleUpsertGrade(const Request& req, Response& res) {
    SessionInfo session;
    if (!requireAdmin(req, res, session)) return;

    std::map<std::string, std::string> form = parseFormBody(req.body);
    std::string exam_id = trim(formValue(form, "exam_id"));
    std::string student_id = trim(formValue(form, "student_id"));
    std::string scores_text = trim(formValue(form, "scores"));

    if (exam_id.empty() || student_id.empty() || scores_text.empty()) {
        setError(res, 400, "考试、学生和成绩不能为空");
        return;
    }

    std::vector<Student> students = loadStudents();
    std::vector<Exam> exams = loadExams();
    std::vector<GradeRecord> grades = loadGrades();
    int student_index = findStudentIndex(students, student_id);
    int exam_index = findExamIndex(exams, exam_id);
    if (student_index < 0) {
        setError(res, 404, "学生不存在");
        return;
    }
    if (exam_index < 0) {
        setError(res, 404, "考试不存在");
        return;
    }

    GradeRecord record;
    record.exam_id = exam_id;
    record.student_id = student_id;
    record.scores = normalizeGradeScoresForExam(exams[static_cast<std::size_t>(exam_index)],
                                                parseGradeScores(scores_text));
    record.updated_at = currentTimeText();

    int grade_index = findGradeIndex(grades, exam_id, student_id);
    if (grade_index >= 0) {
        grades[static_cast<std::size_t>(grade_index)] = record;
    } else {
        grades.push_back(record);
    }
    saveGrades(grades);

    addLog(session.user_id, "保存成绩", "保存成绩 " + exam_id + " / " + student_id);
    setJson(res, makeResponse(true, "成绩保存成功", "null"));
}

void handleDeleteGrade(const Request& req, Response& res, const std::string& exam_id, const std::string& student_id) {
    SessionInfo session;
    if (!requireAdmin(req, res, session)) return;

    std::vector<GradeRecord> grades = loadGrades();
    int grade_index = findGradeIndex(grades, exam_id, student_id);
    if (grade_index < 0) {
        setError(res, 404, "成绩记录不存在");
        return;
    }

    grades.erase(grades.begin() + grade_index);
    saveGrades(grades);
    addLog(session.user_id, "删除成绩", "删除成绩 " + exam_id + " / " + student_id);
    setJson(res, makeResponse(true, "成绩已删除", "null"));
}

void handleStudentGrades(const Request& req, Response& res, const std::string& student_id) {
    SessionInfo session;
    if (!requireSelfOrAdmin(req, res, student_id, session)) return;

    std::vector<Student> students = loadStudents();
    std::vector<Subject> subjects = loadSubjects();
    std::vector<Exam> exams = loadExams();
    std::vector<GradeRecord> grades = loadGrades();
    int student_index = findStudentIndex(students, student_id);
    if (student_index < 0) {
        setError(res, 404, "学生不存在");
        return;
    }

    std::vector<std::string> items;
    for (std::size_t i = 0; i < grades.size(); ++i) {
        if (grades[i].student_id != student_id) continue;
        int exam_index = findExamIndex(exams, grades[i].exam_id);
        if (exam_index < 0) continue;

        std::vector<RankItem> rank_items = buildRankItems(grades[i].exam_id, "total", "", students, exams, grades);
        int class_rank = 0;
        int grade_rank = 0;
        for (std::size_t k = 0; k < rank_items.size(); ++k) {
            if (rank_items[k].student.id == student_id) {
                class_rank = rank_items[k].class_rank;
                grade_rank = rank_items[k].grade_rank;
                break;
            }
        }

        items.push_back(serializeGradeRecord(grades[i],
                                            students[static_cast<std::size_t>(student_index)],
                                            exams[static_cast<std::size_t>(exam_index)],
                                            subjects,
                                            class_rank,
                                            grade_rank));
    }

    std::vector<std::string> fields;
    fields.push_back(fieldRaw("student", serializeStudent(students[static_cast<std::size_t>(student_index)])));
    fields.push_back(fieldRaw("items", makeArray(items)));
    setJson(res, makeResponse(true, "ok", makeObject(fields)));
}

void handleGradeRank(const Request& req, Response& res) {
    SessionInfo session;
    if (!requireAuth(req, res, session)) return;

    std::string exam_id = trim(queryValue(req, "exam_id"));
    std::string sort_mode = trim(queryValue(req, "sort_mode"));
    std::string subject_name = trim(queryValue(req, "subject"));
    std::string student_id = trim(queryValue(req, "student_id"));
    if (sort_mode.empty()) sort_mode = "total";
    if (session.role == "student" && student_id.empty()) {
        student_id = session.user_id;
    }

    if (exam_id.empty()) {
        setError(res, 400, "请先选择考试");
        return;
    }
    if (session.role == "student" && !student_id.empty() && student_id != session.user_id) {
        setError(res, 403, "只能查看自己的排名");
        return;
    }

    std::vector<Student> students = loadStudents();
    std::vector<Exam> exams = loadExams();
    std::vector<GradeRecord> grades = loadGrades();
    int exam_index = findExamIndex(exams, exam_id);
    if (exam_index < 0) {
        setError(res, 404, "考试不存在");
        return;
    }

    std::vector<RankItem> items = buildRankItems(exam_id, sort_mode, subject_name, students, exams, grades);
    std::vector<std::string> item_json;
    std::string current_json = "null";
    for (std::size_t i = 0; i < items.size(); ++i) {
        item_json.push_back(serializeRankItem(items[i], exams[static_cast<std::size_t>(exam_index)]));
        if (!student_id.empty() && items[i].student.id == student_id) {
            current_json = serializeRankItem(items[i], exams[static_cast<std::size_t>(exam_index)]);
        }
    }

    std::vector<std::string> fields;
    fields.push_back(fieldRaw("exam", serializeExam(exams[static_cast<std::size_t>(exam_index)])));
    fields.push_back(fieldRaw("items", makeArray(item_json)));
    fields.push_back(fieldRaw("current", current_json));
    setJson(res, makeResponse(true, "ok", makeObject(fields)));
}

void handleClassRank(const Request& req, Response& res) {
    SessionInfo session;
    if (!requireAuth(req, res, session)) return;

    std::string exam_id = trim(queryValue(req, "exam_id"));
    std::string sort_mode = trim(queryValue(req, "sort_mode"));
    std::string subject_name = trim(queryValue(req, "subject"));
    std::string student_id = trim(queryValue(req, "student_id"));
    std::string class_name = trim(queryValue(req, "class_name"));
    if (sort_mode.empty()) sort_mode = "total";
    if (session.role == "student" && student_id.empty()) {
        student_id = session.user_id;
    }

    if (exam_id.empty()) {
        setError(res, 400, "请先选择考试");
        return;
    }

    std::vector<Student> students = loadStudents();
    std::vector<Exam> exams = loadExams();
    std::vector<GradeRecord> grades = loadGrades();
    int exam_index = findExamIndex(exams, exam_id);
    if (exam_index < 0) {
        setError(res, 404, "考试不存在");
        return;
    }

    if (!student_id.empty()) {
        if (session.role == "student" && student_id != session.user_id) {
            setError(res, 403, "只能查看自己的排名");
            return;
        }
        int student_index = findStudentIndex(students, student_id);
        if (student_index < 0) {
            setError(res, 404, "学生不存在");
            return;
        }
        if (class_name.empty()) {
            class_name = students[static_cast<std::size_t>(student_index)].class_name;
        }
    }

    if (class_name.empty()) {
        setError(res, 400, "请先选择班级");
        return;
    }

    std::vector<RankItem> all_items = buildRankItems(exam_id, sort_mode, subject_name, students, exams, grades);
    std::vector<std::string> item_json;
    std::string current_json = "null";
    for (std::size_t i = 0; i < all_items.size(); ++i) {
        if (all_items[i].student.class_name != class_name) continue;
        item_json.push_back(serializeRankItem(all_items[i], exams[static_cast<std::size_t>(exam_index)]));
        if (!student_id.empty() && all_items[i].student.id == student_id) {
            current_json = serializeRankItem(all_items[i], exams[static_cast<std::size_t>(exam_index)]);
        }
    }

    std::vector<std::string> fields;
    fields.push_back(fieldRaw("exam", serializeExam(exams[static_cast<std::size_t>(exam_index)])));
    fields.push_back(fieldString("class_name", class_name));
    fields.push_back(fieldRaw("items", makeArray(item_json)));
    fields.push_back(fieldRaw("current", current_json));
    setJson(res, makeResponse(true, "ok", makeObject(fields)));
}

void handleExamStats(const Request& req, Response& res) {
    SessionInfo session;
    if (!requireAuth(req, res, session)) return;

    std::string exam_id = trim(queryValue(req, "exam_id"));
    if (exam_id.empty()) {
        setJson(res, makeResponse(true, "ok", buildDashboardJson()));
        return;
    }

    std::vector<Student> students = loadStudents();
    std::vector<Subject> subjects = loadSubjects();
    std::vector<Exam> exams = loadExams();
    std::vector<GradeRecord> grades = loadGrades();
    int exam_index = findExamIndex(exams, exam_id);
    if (exam_index < 0) {
        setError(res, 404, "考试不存在");
        return;
    }

    const Exam& exam = exams[static_cast<std::size_t>(exam_index)];
    std::vector<RankItem> items = buildRankItems(exam_id, "total", "", students, exams, grades);
    double highest_total = 0.0;
    double average_total = 0.0;
    for (std::size_t i = 0; i < items.size(); ++i) {
        highest_total = std::max(highest_total, items[i].total);
        average_total += items[i].total;
    }
    if (!items.empty()) average_total /= static_cast<double>(items.size());

    std::vector<std::string> subject_items;
    for (std::size_t i = 0; i < exam.subjects.size(); ++i) {
        const std::string& subject_name = exam.subjects[i];
        double sum = 0.0;
        int count = 0;
        for (std::size_t j = 0; j < items.size(); ++j) {
            std::map<std::string, std::string>::const_iterator score_it = items[j].grade.scores.find(subject_name);
            if (score_it != items[j].grade.scores.end() && !scoreIsAbsent(score_it->second)) {
                sum += parseDouble(score_it->second);
                ++count;
            }
        }
        std::vector<std::string> fields;
        fields.push_back(fieldString("subject", subject_name));
        fields.push_back(fieldNumber("average", count == 0 ? 0.0 : sum / static_cast<double>(count)));
        fields.push_back(fieldInteger("full_score", fullScoreForSubject(subjects, subject_name)));
        subject_items.push_back(makeObject(fields));
    }

    std::vector<std::string> class_items;
    std::vector<std::string> classes = uniqueClasses(students);
    for (std::size_t i = 0; i < classes.size(); ++i) {
        double sum = 0.0;
        int count = 0;
        for (std::size_t j = 0; j < items.size(); ++j) {
            if (items[j].student.class_name == classes[i]) {
                sum += items[j].total;
                ++count;
            }
        }
        std::vector<std::string> fields;
        fields.push_back(fieldString("class_name", classes[i]));
        fields.push_back(fieldNumber("average_total", count == 0 ? 0.0 : sum / static_cast<double>(count)));
        fields.push_back(fieldInteger("student_count", count));
        class_items.push_back(makeObject(fields));
    }

    std::vector<std::string> fields;
    fields.push_back(fieldRaw("exam", serializeExam(exam)));
    fields.push_back(fieldInteger("student_count", static_cast<int>(items.size())));
    fields.push_back(fieldNumber("highest_total", highest_total));
    fields.push_back(fieldNumber("average_total", average_total));
    fields.push_back(fieldRaw("subject_averages", makeArray(subject_items)));
    fields.push_back(fieldRaw("class_averages", makeArray(class_items)));
    setJson(res, makeResponse(true, "ok", makeObject(fields)));
}

void handleExportCsv(const Request& req, Response& res) {
    SessionInfo session;
    if (!requireAdmin(req, res, session)) return;

    std::string exam_id = trim(queryValue(req, "exam_id"));
    if (exam_id.empty()) {
        setError(res, 400, "请先选择考试");
        return;
    }

    std::vector<Student> students = loadStudents();
    std::vector<Exam> exams = loadExams();
    std::vector<GradeRecord> grades = loadGrades();
    int exam_index = findExamIndex(exams, exam_id);
    if (exam_index < 0) {
        setError(res, 404, "考试不存在");
        return;
    }

    const Exam& exam = exams[static_cast<std::size_t>(exam_index)];
    std::vector<RankItem> items = buildRankItems(exam_id, "total", "", students, exams, grades);
    std::ostringstream csv;
    csv << "学号,姓名,班级";
    for (std::size_t i = 0; i < exam.subjects.size(); ++i) {
        csv << "," << exam.subjects[i];
    }
    csv << ",总分,平均分,班级排名,年级排名\n";

    for (std::size_t i = 0; i < items.size(); ++i) {
        csv << items[i].student.id << "," << items[i].student.name << "," << items[i].student.class_name;
        for (std::size_t j = 0; j < exam.subjects.size(); ++j) {
            std::map<std::string, std::string>::const_iterator score_it = items[i].grade.scores.find(exam.subjects[j]);
            std::string score_text = score_it == items[i].grade.scores.end() ? "" : score_it->second;
            csv << "," << (scoreIsAbsent(score_text) ? "缺考" : score_text);
        }
        csv << "," << formatDouble(items[i].total)
            << "," << formatDouble(items[i].average)
            << "," << items[i].class_rank
            << "," << items[i].grade_rank
            << "\n";
    }

    std::vector<std::string> fields;
    fields.push_back(fieldString("filename", exam.name + ".csv"));
    fields.push_back(fieldString("csv", csv.str()));
    setJson(res, makeResponse(true, "导出成功", makeObject(fields)));
}

void handleLogs(const Request& req, Response& res) {
    SessionInfo session;
    if (!requireAdmin(req, res, session)) return;

    std::vector<LogEntry> logs = loadLogs();
    std::vector<std::string> items;
    for (std::size_t i = 0; i < logs.size(); ++i) {
        items.push_back(serializeLog(logs[i]));
    }
    setJson(res, makeResponse(true, "ok", makeObject(std::vector<std::string>{fieldRaw("items", makeArray(items))})));
}

}  // 匿名命名空间结束

