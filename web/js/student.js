(function() {
  'use strict';

  const auth = Common.initAuth('student');
  if (!auth) return;

  const state = {
    profile: null,
    grades: [],
    exams: []
  };

  const titles = {
    'dashboard-section': '我的概览',
    'grades-section': '我的成绩',
    'rankings-section': '我的排名',
    'info-section': '个人信息',
    'password-section': '修改密码'
  };

  function byId(id) {
    return Common.byId(id);
  }

  function html(text) {
    return Common.escapeHtml(text);
  }

  function setIdentity() {
    byId('studentName').textContent = auth.name || '学生';
    byId('studentAvatar').textContent = (auth.name || '学').charAt(0);
  }

  function bindPageTitles() {
    const links = document.querySelectorAll('.sidebar-nav [data-section]');
    links.forEach(function(link) {
      link.addEventListener('click', function() {
        const sectionId = this.getAttribute('data-section');
        byId('studentPageTitle').textContent = titles[sectionId] || '学生中心';
      });
    });
  }

  function scoreSummary(item) {
    if (!item.scores || !item.scores.length) return '暂无成绩';
    return item.scores.map(function(score) {
      return html(score.subject) + '：' + html(score.score_text);
    }).join('<br>');
  }

  async function loadProfile() {
    const result = await API.students.get(auth.id);
    if (!result.success) {
      Common.showToast(result.message || '加载个人信息失败', 'error');
      return;
    }
    state.profile = result.data;
    byId('welcomeText').textContent =
      '你好，' + state.profile.name + '。你目前所在班级是 ' + state.profile.class_name + '，可以在这里查看自己的成绩和排名。';
    Common.fillForm(byId('studentInfoForm'), state.profile);
    byId('studentInfoId').value = state.profile.id;
    byId('studentInfoClass').value = state.profile.class_name;
  }

  function renderDashboardSummary() {
    const container = byId('dashboardSummary');
    if (!state.grades.length) {
      container.innerHTML = [
        '<div class="summary-card"><div class="label">最近考试总分</div><div class="value">-</div></div>',
        '<div class="summary-card"><div class="label">最近考试平均分</div><div class="value">-</div></div>',
        '<div class="summary-card"><div class="label">班级排名</div><div class="value">-</div></div>',
        '<div class="summary-card"><div class="label">年级排名</div><div class="value">-</div></div>'
      ].join('');
      return;
    }

    const latest = state.grades.reduce(function(best, item) {
      if (!best) return item;
      return item.exam_date > best.exam_date ? item : best;
    }, null);

    container.innerHTML = [
      '<div class="summary-card"><div class="label">最近考试总分</div><div class="value">' + html(latest.total) + '</div></div>',
      '<div class="summary-card"><div class="label">最近考试平均分</div><div class="value">' + html(latest.average) + '</div></div>',
      '<div class="summary-card"><div class="label">班级排名</div><div class="value">' + html(latest.class_rank || '-') + '</div></div>',
      '<div class="summary-card"><div class="label">年级排名</div><div class="value">' + html(latest.grade_rank || '-') + '</div></div>'
    ].join('');
  }

  function renderGrades() {
    const body = byId('studentGradesBody');
    if (!state.grades.length) {
      body.innerHTML = '<tr><td colspan="7" class="empty-state">暂无成绩数据</td></tr>';
      return;
    }

    const sorted = state.grades.slice().sort(function(left, right) {
      if (left.exam_date === right.exam_date) return left.exam_id.localeCompare(right.exam_id);
      return left.exam_date < right.exam_date ? -1 : 1;
    });

    body.innerHTML = sorted.map(function(item) {
      return '<tr>' +
        '<td>' + html(item.exam_name) + '</td>' +
        '<td>' + html(item.exam_date) + '</td>' +
        '<td>' + scoreSummary(item) + '</td>' +
        '<td>' + html(item.total) + '</td>' +
        '<td>' + html(item.average) + '</td>' +
        '<td>' + html(item.class_rank || '-') + '</td>' +
        '<td>' + html(item.grade_rank || '-') + '</td>' +
        '</tr>';
    }).join('');
  }

  function fillExamSelect() {
    const select = byId('studentRankExam');
    const oldValue = select.value;
    let options = '<option value="">请选择考试</option>';
    state.exams.forEach(function(exam) {
      options += '<option value="' + html(exam.exam_id) + '">' + html(exam.exam_name) + '</option>';
    });
    select.innerHTML = options;
    if (oldValue) select.value = oldValue;
  }

  async function loadGrades() {
    const result = await API.grades.student(auth.id);
    if (!result.success) {
      Common.showToast(result.message || '加载成绩失败', 'error');
      return;
    }
    state.grades = result.data.items || [];
    state.exams = state.grades.map(function(item) {
      return {
        exam_id: item.exam_id,
        exam_name: item.exam_name,
        exam_date: item.exam_date
      };
    });
    renderDashboardSummary();
    renderGrades();
    fillExamSelect();
  }

  function renderRankCards(gradeCurrent, classCurrent) {
    const total = gradeCurrent && gradeCurrent.total ? gradeCurrent.total : '-';
    const average = gradeCurrent && gradeCurrent.average ? gradeCurrent.average : '-';
    const classRank = classCurrent && classCurrent.class_rank ? classCurrent.class_rank : '-';
    const gradeRank = gradeCurrent && gradeCurrent.grade_rank ? gradeCurrent.grade_rank : '-';
    byId('studentRankCards').innerHTML = [
      '<div class="summary-card"><div class="label">总分</div><div class="value">' + html(total) + '</div></div>',
      '<div class="summary-card"><div class="label">平均分</div><div class="value">' + html(average) + '</div></div>',
      '<div class="summary-card"><div class="label">班级排名</div><div class="value">' + html(classRank) + '</div></div>',
      '<div class="summary-card"><div class="label">年级排名</div><div class="value">' + html(gradeRank) + '</div></div>'
    ].join('');
  }

  async function loadRankInfo() {
    const examId = byId('studentRankExam').value;
    if (!examId) {
      Common.showToast('请先选择考试', 'error');
      return;
    }

    const gradeResult = await API.rank.grade({
      exam_id: examId,
      student_id: auth.id
    });
    const classResult = await API.rank.classRank({
      exam_id: examId,
      student_id: auth.id
    });

    if (!gradeResult.success || !classResult.success) {
      Common.showToast((gradeResult.message || classResult.message) || '加载排名失败', 'error');
      return;
    }

    const gradeCurrent = gradeResult.data.current;
    const classCurrent = classResult.data.current;
    renderRankCards(gradeCurrent, classCurrent);

    byId('classRankInfo').innerHTML = classCurrent
      ? '<div><div class="label">所在班级</div><div style="font-size:22px;font-weight:700">' + html(classCurrent.class_name) + '</div></div>' +
        '<div style="margin-top:12px" class="table-note">你在本次考试中的班级名次为 <strong>' + html(classCurrent.class_rank) + '</strong>。</div>'
      : '<div class="empty-state">暂无班级排名数据</div>';

    byId('gradeRankInfo').innerHTML = gradeCurrent
      ? '<div><div class="label">所在年级</div><div style="font-size:22px;font-weight:700">' + html(gradeCurrent.grade_name) + '</div></div>' +
        '<div style="margin-top:12px" class="table-note">你在本次考试中的年级名次为 <strong>' + html(gradeCurrent.grade_rank) + '</strong>。</div>'
      : '<div class="empty-state">暂无年级排名数据</div>';
  }

  async function submitInfo(event) {
    event.preventDefault();
    const form = byId('studentInfoForm');
    const data = Common.formDataObject(form);
    delete data.id;
    delete data.class_name;
    const result = await API.students.update(auth.id, data);
    if (!result.success) {
      Common.showToast(result.message || '保存个人信息失败', 'error');
      return;
    }
    Common.showToast(result.message || '保存成功', 'success');
    await loadProfile();
  }

  async function submitPassword(event) {
    event.preventDefault();
    const form = byId('studentPasswordForm');
    const data = Common.formDataObject(form);
    const result = await API.changePassword(data.old_password || '', data.new_password || '');
    if (!result.success) {
      Common.showToast(result.message || '修改密码失败', 'error');
      return;
    }
    form.reset();
    Common.showToast(result.message || '修改成功', 'success');
  }

  function bindEvents() {
    byId('btnLoadStudentRank').addEventListener('click', loadRankInfo);
    byId('studentInfoForm').addEventListener('submit', submitInfo);
    byId('studentPasswordForm').addEventListener('submit', submitPassword);
  }

  async function init() {
    setIdentity();
    bindPageTitles();
    bindEvents();
    await loadProfile();
    await loadGrades();
  }

  init();
})();
