(function() {
  'use strict';

  const auth = Common.initAuth('admin');
  if (!auth) return;

  const state = {
    students: [],
    subjects: [],
    exams: [],
    classes: [],
    grades: []
  };

  const titles = {
    'dashboard-section': '首页概览',
    'students-section': '学生管理',
    'subjects-section': '科目管理',
    'exams-section': '考试管理',
    'grades-section': '成绩管理',
    'rankings-section': '排名统计',
    'password-section': '修改密码'
  };

  function byId(id) {
    return Common.byId(id);
  }

  function html(text) {
    return Common.escapeHtml(text);
  }

  function setAdminIdentity() {
    byId('adminName').textContent = auth.name || '管理员';
    byId('adminAvatar').textContent = (auth.name || 'A').charAt(0);
  }

  function bindPageTitles() {
    const links = document.querySelectorAll('.sidebar-nav [data-section]');
    links.forEach(function(link) {
      link.addEventListener('click', function() {
        const sectionId = this.getAttribute('data-section');
        byId('pageTitle').textContent = titles[sectionId] || '管理后台';
      });
    });
  }

  function selectedExamById(examId) {
    return state.exams.find(function(exam) { return exam.id === examId; }) || null;
  }

  function selectedStudentById(studentId) {
    return state.students.find(function(student) { return student.id === studentId; }) || null;
  }

  function scoreSummary(item) {
    if (!item.scores || !item.scores.length) return '暂无成绩';
    return item.scores.map(function(score) {
      return html(score.subject) + '：' + html(score.score_text);
    }).join('<br>');
  }

  function fillClassSelect(selectId, includeEmpty) {
    const select = byId(selectId);
    if (!select) return;
    const oldValue = select.value;
    let options = includeEmpty ? '<option value="">全部班级</option>' : '<option value="">请选择班级</option>';
    state.classes.forEach(function(item) {
      options += '<option value="' + html(item.class_name) + '">' + html(item.class_name) + '</option>';
    });
    select.innerHTML = options;
    if (oldValue) select.value = oldValue;
  }

  function fillStudentSelect(selectId) {
    const select = byId(selectId);
    if (!select) return;
    const oldValue = select.value;
    let options = '<option value="">请选择学生</option>';
    state.students.forEach(function(student) {
      options += '<option value="' + html(student.id) + '">' + html(student.id + ' - ' + student.name) + '</option>';
    });
    select.innerHTML = options;
    if (oldValue) select.value = oldValue;
  }

  function fillExamSelect(selectId, placeholder) {
    const select = byId(selectId);
    if (!select) return;
    const oldValue = select.value;
    let options = '<option value="">' + html(placeholder || '请选择考试') + '</option>';
    state.exams.forEach(function(exam) {
      options += '<option value="' + html(exam.id) + '">' + html(exam.name) + '</option>';
    });
    select.innerHTML = options;
    if (oldValue) select.value = oldValue;
  }

  function fillRankSubjectSelect() {
    const select = byId('rankSubjectSelect');
    if (!select) return;
    const exam = selectedExamById(byId('rankExamSelect').value);
    const oldValue = select.value;
    let options = '<option value="">请选择科目</option>';
    if (exam && exam.subjects) {
      exam.subjects.forEach(function(subject) {
        options += '<option value="' + html(subject) + '">' + html(subject) + '</option>';
      });
    }
    select.innerHTML = options;
    if (oldValue) select.value = oldValue;
  }

  function renderDashboard(data) {
    byId('statStudents').textContent = data.student_count || 0;
    byId('statClasses').textContent = data.class_count || 0;
    byId('statSubjects').textContent = data.subject_count || 0;
    byId('statExams').textContent = data.exam_count || 0;
    byId('statGrades').textContent = data.grade_count || 0;

    byId('latestExamTitle').textContent = data.latest_exam
      ? ('最近考试：' + data.latest_exam.name + '（' + data.latest_exam.date + '）')
      : '暂无考试数据';

    const topBody = byId('topStudentsBody');
    if (!data.top_students || !data.top_students.length) {
      topBody.innerHTML = '<tr><td colspan="6" class="empty-state">暂无排名数据</td></tr>';
    } else {
      topBody.innerHTML = data.top_students.map(function(item) {
        return '<tr>' +
          '<td>' + html(item.grade_rank) + '</td>' +
          '<td>' + html(item.student_id) + '</td>' +
          '<td>' + html(item.student_name) + '</td>' +
          '<td>' + html(item.class_name) + '</td>' +
          '<td>' + html(item.total) + '</td>' +
          '<td>' + html(item.average) + '</td>' +
          '</tr>';
      }).join('');
    }

    const recentLogs = byId('recentLogs');
    if (!data.recent_logs || !data.recent_logs.length) {
      recentLogs.innerHTML = '<div class="empty-state">暂无日志</div>';
    } else {
      recentLogs.innerHTML = data.recent_logs.slice().reverse().map(function(log) {
        return '<div class="summary-box">' +
          '<div><strong>' + html(log.action) + '</strong></div>' +
          '<div class="table-note">' + html(log.user_id) + ' · ' + html(log.timestamp) + '</div>' +
          '<div style="margin-top:6px">' + html(log.detail) + '</div>' +
          '</div>';
      }).join('');
    }
  }

  async function loadDashboard() {
    const result = await API.stats.overview();
    if (!result.success) {
      Common.showToast(result.message || '加载概览失败', 'error');
      return;
    }
    renderDashboard(result.data || {});
  }

  async function loadClasses() {
    const result = await API.classes.list();
    if (!result.success) {
      Common.showToast(result.message || '加载班级失败', 'error');
      return;
    }
    state.classes = result.data.items || [];
    fillClassSelect('studentClassFilter', true);
    fillClassSelect('gradeClassFilter', true);
    fillClassSelect('rankClassSelect', false);
  }

  function renderStudentTable() {
    const body = byId('studentTableBody');
    if (!state.students.length) {
      body.innerHTML = '<tr><td colspan="6" class="empty-state">暂无学生数据</td></tr>';
      return;
    }
    body.innerHTML = state.students.map(function(student) {
      return '<tr>' +
        '<td>' + html(student.id) + '</td>' +
        '<td>' + html(student.name) + '</td>' +
        '<td>' + html(student.gender) + '</td>' +
        '<td>' + html(student.class_name) + '</td>' +
        '<td>' + html(student.grade_name) + '</td>' +
        '<td>' +
        '<button class="btn btn-outline btn-sm" onclick="AdminPage.editStudent(\'' + encodeURIComponent(student.id) + '\')">编辑</button> ' +
        '<button class="btn btn-outline btn-sm" onclick="AdminPage.deleteStudent(\'' + encodeURIComponent(student.id) + '\')">删除</button>' +
        '</td>' +
        '</tr>';
    }).join('');
  }

  async function loadStudents() {
    const params = {
      search: byId('studentSearch').value.trim(),
      class_name: byId('studentClassFilter').value
    };
    const result = await API.students.list(params);
    if (!result.success) {
      Common.showToast(result.message || '加载学生失败', 'error');
      return;
    }
    state.students = result.data.items || [];
    renderStudentTable();
    fillStudentSelect('gradeFormStudent');
  }

  function resetStudentForm() {
    const form = byId('studentForm');
    form.reset();
    byId('studentEditId').value = '';
    byId('studentIdInput').readOnly = false;
    byId('studentFormTitle').textContent = '新增学生';
  }

  async function submitStudent(event) {
    event.preventDefault();
    const form = byId('studentForm');
    const data = Common.formDataObject(form);
    const editId = byId('studentEditId').value;

    let result;
    if (editId) {
      result = await API.students.update(editId, data);
    } else {
      result = await API.students.add(data);
    }

    if (!result.success) {
      Common.showToast(result.message || '保存学生失败', 'error');
      return;
    }

    Common.showToast(result.message || '保存成功', 'success');
    resetStudentForm();
    await loadClasses();
    await loadStudents();
    await loadDashboard();
  }

  function editStudent(encodedId) {
    const id = decodeURIComponent(encodedId);
    const student = selectedStudentById(id);
    if (!student) return;
    Common.fillForm(byId('studentForm'), student);
    byId('studentEditId').value = id;
    byId('studentIdInput').value = student.id;
    byId('studentIdInput').readOnly = true;
    byId('studentFormTitle').textContent = '编辑学生';
    Common.navigateTo('students-section');
    byId('pageTitle').textContent = titles['students-section'];
  }

  async function deleteStudent(encodedId) {
    const id = decodeURIComponent(encodedId);
    if (!window.confirm('确认删除学生 ' + id + ' 吗？')) return;
    const result = await API.students.remove(id);
    if (!result.success) {
      Common.showToast(result.message || '删除学生失败', 'error');
      return;
    }
    Common.showToast(result.message || '删除成功', 'success');
    await loadClasses();
    await loadStudents();
    await loadDashboard();
  }

  function renderSubjectTable() {
    const body = byId('subjectTableBody');
    if (!state.subjects.length) {
      body.innerHTML = '<tr><td colspan="4" class="empty-state">暂无科目数据</td></tr>';
      return;
    }
    body.innerHTML = state.subjects.map(function(subject) {
      return '<tr>' +
        '<td>' + html(subject.id) + '</td>' +
        '<td>' + html(subject.name) + '</td>' +
        '<td>' + html(subject.full_score) + '</td>' +
        '<td>' +
        '<button class="btn btn-outline btn-sm" onclick="AdminPage.editSubject(\'' + encodeURIComponent(subject.id) + '\')">编辑</button> ' +
        '<button class="btn btn-outline btn-sm" onclick="AdminPage.deleteSubject(\'' + encodeURIComponent(subject.id) + '\')">删除</button>' +
        '</td>' +
        '</tr>';
    }).join('');
  }

  async function loadSubjects() {
    const result = await API.subjects.list();
    if (!result.success) {
      Common.showToast(result.message || '加载科目失败', 'error');
      return;
    }
    state.subjects = result.data.items || [];
    renderSubjectTable();
    renderExamSubjectChecklist([]);
    fillRankSubjectSelect();
  }

  function resetSubjectForm() {
    byId('subjectForm').reset();
    byId('subjectEditId').value = '';
    byId('subjectFormTitle').textContent = '新增科目';
  }

  async function submitSubject(event) {
    event.preventDefault();
    const form = byId('subjectForm');
    const data = Common.formDataObject(form);
    const editId = byId('subjectEditId').value;

    const result = editId
      ? await API.subjects.update(editId, data)
      : await API.subjects.add(data);

    if (!result.success) {
      Common.showToast(result.message || '保存科目失败', 'error');
      return;
    }

    Common.showToast(result.message || '保存成功', 'success');
    resetSubjectForm();
    await loadSubjects();
    await loadExams();
    await loadDashboard();
  }

  function editSubject(encodedId) {
    const id = decodeURIComponent(encodedId);
    const subject = state.subjects.find(function(item) { return item.id === id; });
    if (!subject) return;
    Common.fillForm(byId('subjectForm'), subject);
    byId('subjectEditId').value = id;
    byId('subjectFormTitle').textContent = '编辑科目';
    Common.navigateTo('subjects-section');
    byId('pageTitle').textContent = titles['subjects-section'];
  }

  async function deleteSubject(encodedId) {
    const id = decodeURIComponent(encodedId);
    if (!window.confirm('确认删除这个科目吗？')) return;
    const result = await API.subjects.remove(id);
    if (!result.success) {
      Common.showToast(result.message || '删除科目失败', 'error');
      return;
    }
    Common.showToast(result.message || '删除成功', 'success');
    await loadSubjects();
    await loadExams();
    await loadDashboard();
  }

  function renderExamSubjectChecklist(selectedSubjects) {
    const container = byId('examSubjectChecklist');
    if (!container) return;
    if (!state.subjects.length) {
      container.innerHTML = '<span class="table-note">请先添加科目</span>';
      return;
    }
    container.innerHTML = state.subjects.map(function(subject) {
      const checked = selectedSubjects.indexOf(subject.name) >= 0 ? ' checked' : '';
      return '<label><input type="checkbox" name="exam_subject" value="' + html(subject.name) + '"' + checked + '><span>' +
        html(subject.name + '（' + subject.full_score + '分）') + '</span></label>';
    }).join('');
  }

  function selectedExamSubjects() {
    const checks = document.querySelectorAll('#examSubjectChecklist input[name="exam_subject"]:checked');
    const subjects = [];
    checks.forEach(function(check) {
      subjects.push(check.value);
    });
    return subjects;
  }

  function renderExamTable() {
    const body = byId('examTableBody');
    if (!state.exams.length) {
      body.innerHTML = '<tr><td colspan="5" class="empty-state">暂无考试数据</td></tr>';
      return;
    }
    body.innerHTML = state.exams.map(function(exam) {
      return '<tr>' +
        '<td>' + html(exam.id) + '</td>' +
        '<td>' + html(exam.name) + '</td>' +
        '<td>' + html(exam.date) + '</td>' +
        '<td>' + html((exam.subjects || []).join('、')) + '</td>' +
        '<td>' +
        '<button class="btn btn-outline btn-sm" onclick="AdminPage.editExam(\'' + encodeURIComponent(exam.id) + '\')">编辑</button> ' +
        '<button class="btn btn-outline btn-sm" onclick="AdminPage.deleteExam(\'' + encodeURIComponent(exam.id) + '\')">删除</button>' +
        '</td>' +
        '</tr>';
    }).join('');
  }

  async function loadExams() {
    const result = await API.exams.list();
    if (!result.success) {
      Common.showToast(result.message || '加载考试失败', 'error');
      return;
    }
    state.exams = result.data.items || [];
    renderExamTable();
    fillExamSelect('gradeExamFilter', '请选择考试');
    fillExamSelect('gradeFormExam', '请选择考试');
    fillExamSelect('rankExamSelect', '请选择考试');
    fillRankSubjectSelect();
  }

  function resetExamForm() {
    byId('examForm').reset();
    byId('examEditId').value = '';
    byId('examFormTitle').textContent = '新增考试';
    renderExamSubjectChecklist([]);
  }

  async function submitExam(event) {
    event.preventDefault();
    const data = Common.formDataObject(byId('examForm'));
    data.subjects = selectedExamSubjects().join(',');
    const editId = byId('examEditId').value;

    const result = editId
      ? await API.exams.update(editId, data)
      : await API.exams.add(data);

    if (!result.success) {
      Common.showToast(result.message || '保存考试失败', 'error');
      return;
    }

    Common.showToast(result.message || '保存成功', 'success');
    resetExamForm();
    await loadExams();
    await loadDashboard();
  }

  function editExam(encodedId) {
    const id = decodeURIComponent(encodedId);
    const exam = selectedExamById(id);
    if (!exam) return;
    Common.fillForm(byId('examForm'), exam);
    byId('examEditId').value = id;
    byId('examFormTitle').textContent = '编辑考试';
    renderExamSubjectChecklist(exam.subjects || []);
    Common.navigateTo('exams-section');
    byId('pageTitle').textContent = titles['exams-section'];
  }

  async function deleteExam(encodedId) {
    const id = decodeURIComponent(encodedId);
    if (!window.confirm('确认删除这个考试吗？关联成绩也会被删除。')) return;
    const result = await API.exams.remove(id);
    if (!result.success) {
      Common.showToast(result.message || '删除考试失败', 'error');
      return;
    }
    Common.showToast(result.message || '删除成功', 'success');
    await loadExams();
    await loadDashboard();
    state.grades = [];
    renderGradeTable();
  }

  function renderGradeInputs(examId, scoreMap) {
    const container = byId('gradeInputs');
    const exam = selectedExamById(examId);
    if (!exam) {
      container.innerHTML = '<div class="table-note">请选择考试后录入成绩。</div>';
      return;
    }
    container.innerHTML = (exam.subjects || []).map(function(subject) {
      const value = scoreMap && scoreMap[subject] ? scoreMap[subject] : '';
      const displayValue = value === 'ABS' ? '' : value;
      return '<div class="form-group">' +
        '<label>' + html(subject) + '</label>' +
        '<input type="number" data-subject="' + html(subject) + '" placeholder="留空表示缺考" value="' + html(displayValue) + '">' +
        '</div>';
    }).join('');
  }

  function renderGradeTable() {
    const body = byId('gradeTableBody');
    if (!state.grades.length) {
      body.innerHTML = '<tr><td colspan="7" class="empty-state">暂无成绩记录</td></tr>';
      return;
    }
    body.innerHTML = state.grades.map(function(item) {
      return '<tr>' +
        '<td>' + html(item.student_id) + '</td>' +
        '<td>' + html(item.student_name) + '</td>' +
        '<td>' + html(item.class_name) + '</td>' +
        '<td>' + scoreSummary(item) + '</td>' +
        '<td>' + html(item.total) + '</td>' +
        '<td>' + html(item.average) + '</td>' +
        '<td>' +
        '<button class="btn btn-outline btn-sm" onclick="AdminPage.editGrade(\'' + encodeURIComponent(item.exam_id) + '\', \'' + encodeURIComponent(item.student_id) + '\')">编辑</button> ' +
        '<button class="btn btn-outline btn-sm" onclick="AdminPage.deleteGrade(\'' + encodeURIComponent(item.exam_id) + '\', \'' + encodeURIComponent(item.student_id) + '\')">删除</button>' +
        '</td>' +
        '</tr>';
    }).join('');
  }

  async function loadGrades() {
    const examId = byId('gradeExamFilter').value;
    if (!examId) {
      state.grades = [];
      renderGradeTable();
      return;
    }
    const params = { exam_id: examId };
    const className = byId('gradeClassFilter').value;
    if (className) params.class_name = className;

    const result = await API.grades.list(params);
    if (!result.success) {
      Common.showToast(result.message || '加载成绩失败', 'error');
      return;
    }
    state.grades = result.data.items || [];
    renderGradeTable();
  }

  function resetGradeForm() {
    byId('gradeForm').reset();
    byId('gradeEditingKey').value = '';
    byId('gradeFormTitle').textContent = '录入或修改成绩';
    renderGradeInputs(byId('gradeFormExam').value, null);
  }

  async function submitGrade(event) {
    event.preventDefault();
    const examId = byId('gradeFormExam').value;
    const studentId = byId('gradeFormStudent').value;
    if (!examId || !studentId) {
      Common.showToast('请选择考试和学生', 'error');
      return;
    }

    const inputs = byId('gradeInputs').querySelectorAll('input[data-subject]');
    const scoreParts = [];
    inputs.forEach(function(input) {
      const subject = input.getAttribute('data-subject');
      const value = input.value.trim();
      scoreParts.push(subject + ':' + (value || 'ABS'));
    });

    const result = await API.grades.save({
      exam_id: examId,
      student_id: studentId,
      scores: scoreParts.join(',')
    });

    if (!result.success) {
      Common.showToast(result.message || '保存成绩失败', 'error');
      return;
    }

    Common.showToast(result.message || '保存成功', 'success');
    byId('gradeExamFilter').value = examId;
    await loadGrades();
    await loadDashboard();
    resetGradeForm();
  }

  function editGrade(encodedExamId, encodedStudentId) {
    const examId = decodeURIComponent(encodedExamId);
    const studentId = decodeURIComponent(encodedStudentId);
    const item = state.grades.find(function(grade) {
      return grade.exam_id === examId && grade.student_id === studentId;
    });
    if (!item) return;

    byId('gradeFormExam').value = examId;
    byId('gradeFormStudent').value = studentId;
    byId('gradeEditingKey').value = examId + '|' + studentId;
    byId('gradeFormTitle').textContent = '编辑成绩';
    renderGradeInputs(examId, item.score_map || {});
    Common.navigateTo('grades-section');
    byId('pageTitle').textContent = titles['grades-section'];
  }

  async function deleteGrade(encodedExamId, encodedStudentId) {
    const examId = decodeURIComponent(encodedExamId);
    const studentId = decodeURIComponent(encodedStudentId);
    if (!window.confirm('确认删除这条成绩记录吗？')) return;
    const result = await API.grades.remove(examId, studentId);
    if (!result.success) {
      Common.showToast(result.message || '删除成绩失败', 'error');
      return;
    }
    Common.showToast(result.message || '删除成功', 'success');
    await loadGrades();
    await loadDashboard();
  }

  function renderRankSummary(statsData, scope) {
    const cards = byId('rankSummaryCards');
    if (!statsData) {
      cards.innerHTML = '<div class="summary-box">请选择考试后查看统计</div>';
      return;
    }
    const classCount = (statsData.class_averages || []).length;
    cards.innerHTML = [
      '<div class="summary-box"><div class="table-note">参考人数</div><div style="font-size:28px;font-weight:700">' + html(statsData.student_count || 0) + '</div></div>',
      '<div class="summary-box"><div class="table-note">最高总分</div><div style="font-size:28px;font-weight:700">' + html(statsData.highest_total || 0) + '</div></div>',
      '<div class="summary-box"><div class="table-note">平均总分</div><div style="font-size:28px;font-weight:700">' + html(statsData.average_total || 0) + '</div></div>',
      '<div class="summary-box"><div class="table-note">' + (scope === 'class' ? '参与班级' : '统计班级') + '</div><div style="font-size:28px;font-weight:700">' + html(classCount) + '</div></div>'
    ].join('');
  }

  async function loadRankings() {
    const examId = byId('rankExamSelect').value;
    if (!examId) {
      Common.showToast('请先选择考试', 'error');
      return;
    }

    const scope = byId('rankScope').value;
    const sortMode = byId('rankMode').value;
    const subject = byId('rankSubjectSelect').value;
    const className = byId('rankClassSelect').value;

    const rankParams = { exam_id: examId, sort_mode: sortMode };
    if (sortMode === 'subject' && subject) rankParams.subject = subject;
    if (scope === 'class' && className) rankParams.class_name = className;

    const rankResult = scope === 'grade'
      ? await API.rank.grade(rankParams)
      : await API.rank.classRank(rankParams);

    const statsResult = await API.stats.exam(examId);

    if (!rankResult.success) {
      Common.showToast(rankResult.message || '加载排名失败', 'error');
      return;
    }
    if (!statsResult.success) {
      Common.showToast(statsResult.message || '加载统计失败', 'error');
      return;
    }

    renderRankSummary(statsResult.data, scope);

    const body = byId('rankTableBody');
    const items = rankResult.data.items || [];
    if (!items.length) {
      body.innerHTML = '<tr><td colspan="7" class="empty-state">暂无排名数据</td></tr>';
      return;
    }

    body.innerHTML = items.map(function(item) {
      const rankValue = scope === 'grade' ? item.grade_rank : item.class_rank;
      const scoreMap = item.score_map || {};
      const scoreText = Object.keys(scoreMap).map(function(key) {
        const value = scoreMap[key] === 'ABS' ? '缺考' : scoreMap[key];
        return html(key + '：' + value);
      }).join('<br>');
      return '<tr>' +
        '<td>' + html(rankValue) + '</td>' +
        '<td>' + html(item.student_id) + '</td>' +
        '<td>' + html(item.student_name) + '</td>' +
        '<td>' + html(item.class_name) + '</td>' +
        '<td>' + html(item.total) + '</td>' +
        '<td>' + html(item.average) + '</td>' +
        '<td>' + scoreText + '</td>' +
        '</tr>';
    }).join('');
  }

  async function exportCsv() {
    const examId = byId('gradeExamFilter').value;
    if (!examId) {
      Common.showToast('请先选择考试', 'error');
      return;
    }
    const result = await API.exportCsv(examId);
    if (!result.success) {
      Common.showToast(result.message || '导出失败', 'error');
      return;
    }
    Common.downloadText(result.data.filename || 'grades.csv', result.data.csv || '');
    Common.showToast('CSV 已导出', 'success');
  }

  async function submitPassword(event) {
    event.preventDefault();
    const data = Common.formDataObject(byId('passwordForm'));
    const result = await API.changePassword(data.old_password || '', data.new_password || '');
    if (!result.success) {
      Common.showToast(result.message || '修改密码失败', 'error');
      return;
    }
    byId('passwordForm').reset();
    Common.showToast(result.message || '修改成功', 'success');
  }

  function bindEvents() {
    byId('btnStudentSearch').addEventListener('click', loadStudents);
    byId('btnStudentReset').addEventListener('click', function() {
      byId('studentSearch').value = '';
      byId('studentClassFilter').value = '';
      loadStudents();
    });
    byId('studentForm').addEventListener('submit', submitStudent);
    byId('btnStudentCancel').addEventListener('click', resetStudentForm);

    byId('subjectForm').addEventListener('submit', submitSubject);
    byId('btnSubjectCancel').addEventListener('click', resetSubjectForm);

    byId('examForm').addEventListener('submit', submitExam);
    byId('btnExamCancel').addEventListener('click', resetExamForm);

    byId('gradeFormExam').addEventListener('change', function() {
      renderGradeInputs(this.value, null);
    });
    byId('gradeForm').addEventListener('submit', submitGrade);
    byId('btnGradeCancel').addEventListener('click', resetGradeForm);
    byId('btnGradeLoad').addEventListener('click', loadGrades);
    byId('btnExportCsv').addEventListener('click', exportCsv);

    byId('rankExamSelect').addEventListener('change', fillRankSubjectSelect);
    byId('btnRankLoad').addEventListener('click', loadRankings);

    byId('passwordForm').addEventListener('submit', submitPassword);
  }

  async function init() {
    setAdminIdentity();
    bindPageTitles();
    bindEvents();

    await loadClasses();
    await loadSubjects();
    await loadExams();
    await loadStudents();
    await loadDashboard();

    renderGradeInputs('', null);
  }

  window.AdminPage = {
    editStudent: editStudent,
    deleteStudent: deleteStudent,
    editSubject: editSubject,
    deleteSubject: deleteSubject,
    editExam: editExam,
    deleteExam: deleteExam,
    editGrade: editGrade,
    deleteGrade: deleteGrade
  };

  init();
})();
