(function() {
  'use strict';

  var TOKEN_KEY = 'token';
  var USER_ID_KEY = 'user_id';
  var USER_NAME_KEY = 'user_name';
  var USER_ROLE_KEY = 'user_role';
  var THEME_KEY = 'theme';

  function escapeHtml(str) {
    if (!str && str !== 0) return '';
    var div = document.createElement('div');
    div.textContent = String(str);
    return div.innerHTML;
  }

  function showToast(msg, type) {
    var existing = document.getElementById('toast');
    if (existing) existing.remove();
    if (!msg) return;
    var div = document.createElement('div');
    div.id = 'toast';
    div.style.cssText = 'position:fixed;top:20px;right:20px;padding:12px 24px;border-radius:8px;color:#fff;font-size:14px;z-index:9999;box-shadow:0 4px 12px rgba(0,0,0,0.2);animation:slideUp .3s;';
    div.textContent = msg;
    if (type === 'success') div.style.background = '#2ec4b6';
    else if (type === 'error') div.style.background = '#e71d36';
    else if (type === 'warning') div.style.background = '#ff9f1c';
    else div.style.background = '#4361ee';
    document.body.appendChild(div);
    setTimeout(function() { if (div.parentNode) div.remove(); }, 3000);
  }

  function confirm(msg, callback) {
    if (window.confirm(msg)) callback();
  }

  function closeModal(id) {
    var el = typeof id === 'string' ? document.getElementById(id) : id;
    if (!el) return;
    el.style.display = 'none';
  }

  function openModal(id) {
    var el = typeof id === 'string' ? document.getElementById(id) : id;
    if (!el) return;
    el.style.display = 'flex';
    var fi = el.querySelector('input:not([type=hidden])');
    if (fi) setTimeout(function() { fi.focus(); }, 100);
  }

  function getFormValues(selector) {
    var data = {};
    var els = document.querySelectorAll(selector);
    els.forEach(function(el) {
      var name = el.getAttribute('name') || el.id;
      if (!name || name.substr(0, 1) === '_') return;
      if (el.type === 'checkbox') data[name] = el.checked;
      else if (el.type === 'number') data[name] = parseFloat(el.value) || 0;
      else data[name] = el.value;
    });
    return data;
  }

  function setFormValues(selector, obj) {
    if (!obj) return;
    Object.keys(obj).forEach(function(key) {
      var el = document.querySelector(selector + ' [name="' + key + '"], ' + selector + ' #' + key);
      if (!el) return;
      if (el.type === 'checkbox') el.checked = !!obj[key];
      else if (el.type === 'number') el.value = obj[key] || 0;
      else el.value = obj[key] || '';
    });
  }

  function statusBadge(status) {
    var map = { draft: '草稿', published: '已发布', locked: '已锁定' };
    var cls = { draft: 'badge-warning', published: 'badge-success', locked: 'badge-info' };
    return '<span class="badge ' + (cls[status] || 'badge-warning') + '">' + (map[status] || status || '未知') + '</span>';
  }

  function scoreColor(score) {
    if (score === null || score === undefined || isNaN(score)) return '';
    if (score >= 90) return 'score-a';
    if (score >= 80) return 'score-b';
    if (score >= 70) return 'score-c';
    return 'score-d';
  }

  function renderPagination(containerId, page, total, size, onPage) {
    var container = typeof containerId === 'string' ? document.getElementById(containerId) : containerId;
    if (!container) return;
    var totalPages = Math.max(1, Math.ceil(total / (size || 20)));
    var pg = Math.max(1, Math.min(page || 1, totalPages));
    container.innerHTML = '';
    if (totalPages <= 1) return;

    var addBtn = function(txt, pgNum, disabled) {
      var b = document.createElement('button');
      b.textContent = txt;
      b.disabled = disabled || false;
      if (pgNum === pg) b.classList.add('active');
      b.addEventListener('click', function() { onPage(pgNum); });
      container.appendChild(b);
    };

    addBtn('\u2039', pg - 1, pg <= 1);
    var start = Math.max(1, pg - 2);
    var end = Math.min(totalPages, pg + 2);
    if (start > 1) { addBtn('1', 1); if (start > 2) { var s = document.createElement('span'); s.textContent = '...'; container.appendChild(s); } }
    for (var i = start; i <= end; i++) addBtn(String(i), i);
    if (end < totalPages) { if (end < totalPages - 1) { var s2 = document.createElement('span'); s2.textContent = '...'; container.appendChild(s2); } addBtn(String(totalPages), totalPages); }
    addBtn('\u203a', pg + 1, pg >= totalPages);
  }

  function $ (sel) { return document.querySelector(sel); }
  function $$ (sel) { return Array.from(document.querySelectorAll(sel)); }

  function navigateTo(sec) {
    if (!sec) return;
    $$('.page-section').forEach(function(p) { p.classList.remove('active'); });
    var el = $(typeof sec === 'string' ? '#' + sec : null) || sec;
    if (el) el.classList.add('active');
    $$('.sidebar-nav a').forEach(function(a) { a.classList.remove('active'); });
    var navLink = $('.sidebar-nav a[data-section="' + sec + '"]');
    if (navLink) navLink.classList.add('active');
  }

  function getRole() { return localStorage.getItem(USER_ROLE_KEY) || ''; }
  function getUserId() { return localStorage.getItem(USER_ID_KEY) || ''; }
  function getUserName() { return localStorage.getItem(USER_NAME_KEY) || ''; }

  function logout() {
    localStorage.removeItem(TOKEN_KEY);
    localStorage.removeItem(USER_ID_KEY);
    localStorage.removeItem(USER_NAME_KEY);
    localStorage.removeItem(USER_ROLE_KEY);
    window.location.href = '/';
  }

  function initTheme() {
    var theme = localStorage.getItem(THEME_KEY);
    if (!theme) theme = window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light';
    document.documentElement.setAttribute('data-theme', theme);
  }

  function toggleTheme() {
    var cur = document.documentElement.getAttribute('data-theme') || 'light';
    var next = cur === 'dark' ? 'light' : 'dark';
    document.documentElement.setAttribute('data-theme', next);
    localStorage.setItem(THEME_KEY, next);
  }

  window.Util = {
    showToast: showToast,
    closeModal: closeModal,
    openModal: openModal,
    confirm: confirm,
    escapeHtml: escapeHtml,
    setFormValues: setFormValues,
    getFormValues: getFormValues,
    statusBadge: statusBadge,
    scoreColor: scoreColor,
    renderPagination: renderPagination
  };

  window.$ = $;
  window.$$ = $$;
  window.navigateTo = navigateTo;
  window.logout = logout;
  window.toggleTheme = toggleTheme;
  window.getRole = getRole;
  window.getUserId = getUserId;
  window.getUserName = getUserName;
  window.checkAuth = function(requiredRole) {
    var token = localStorage.getItem(TOKEN_KEY);
    var role = localStorage.getItem(USER_ROLE_KEY);
    if (!token || !role) { window.location.href = 'login.html'; return false; }
    if (requiredRole && role !== requiredRole) { window.location.href = 'login.html'; return false; }
    return { token: token, role: role, id: getUserId(), name: getUserName() };
  };

  function initAuth(role) {
    var token = localStorage.getItem(TOKEN_KEY);
    var userRole = localStorage.getItem(USER_ROLE_KEY);
    if (!token || !userRole) { window.location.href = 'login.html'; return false; }
    if (role && userRole !== role) { window.location.href = 'login.html'; return false; }
    API.token = token;
    return true;
  }

  function showLoading(container) {
    var el = typeof container === 'string' ? document.getElementById(container) : container;
    if (!el) return;
    var l = document.createElement('div');
    l.className = 'loading';
    l.innerHTML = '<span class="spinner"></span> 加载中...';
    el.style.position = 'relative';
    el.appendChild(l);
    return function() { if (l.parentNode) l.parentNode.removeChild(l); };
  }

  window.Common = {
    initAuth: initAuth,
    getUserId: getUserId,
    getUserName: getUserName,
    getRole: getRole,
    showLoading: showLoading,
    showToast: showToast,
    escapeHtml: escapeHtml,
    closeModal: closeModal,
    openModal: openModal,
    confirm: confirm,
    setFormValues: setFormValues,
    getFormValues: getFormValues,
    statusBadge: statusBadge,
    scoreColor: scoreColor,
    renderPagination: renderPagination,
    formatDate: function(d) { if (!d) return ''; var dt = new Date(d); if (isNaN(dt.getTime())) return d; return dt.getFullYear() + '-' + String(dt.getMonth()+1).padStart(2,'0') + '-' + String(dt.getDate()).padStart(2,'0'); },
    gradeLabel: function(score) {
      if (score >= 90) return { text: 'A', cls: 'score-a' };
      if (score >= 80) return { text: 'B', cls: 'score-b' };
      if (score >= 70) return { text: 'C', cls: 'score-c' };
      if (score >= 60) return { text: 'D', cls: 'score-d' };
      return { text: 'F', cls: 'score-d' };
    }
  };

  window.initTheme = initTheme;

  document.addEventListener('DOMContentLoaded', function() {
    initTheme();
    var isPublic = window.location.pathname.endsWith('index.html') ||
                   window.location.pathname.endsWith('login.html') ||
                   window.location.pathname === '/' || window.location.pathname.endsWith('/');
    if (!isPublic) {
      var token = localStorage.getItem(TOKEN_KEY);
      var role = localStorage.getItem(USER_ROLE_KEY);
      if (!token || !role) { window.location.href = 'login.html'; return; }
      if (typeof API !== 'undefined') API.token = token;
    }
    document.addEventListener('click', function(e) {
      var overlay = e.target.closest('.modal-overlay');
      if (overlay && e.target === overlay) closeModal(overlay);
    });
  });
})();
