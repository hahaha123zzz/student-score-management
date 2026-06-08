(function() {
  'use strict';

  const TOKEN_KEY = 'token';
  const USER_ID_KEY = 'user_id';
  const USER_NAME_KEY = 'user_name';
  const USER_ROLE_KEY = 'user_role';
  const THEME_KEY = 'theme';

  function byId(id) {
    return document.getElementById(id);
  }

  function escapeHtml(value) {
    if (value === undefined || value === null) return '';
    const div = document.createElement('div');
    div.textContent = String(value);
    return div.innerHTML;
  }

  function showToast(message, type) {
    if (!message) return;
    const oldToast = byId('toast');
    if (oldToast) oldToast.remove();

    const toast = document.createElement('div');
    toast.id = 'toast';
    toast.style.cssText =
      'position:fixed;top:20px;right:20px;z-index:9999;padding:12px 18px;border-radius:10px;' +
      'color:#fff;font-size:14px;box-shadow:0 10px 30px rgba(15,23,42,.18);';
    toast.textContent = message;
    if (type === 'error') toast.style.background = '#dc2626';
    else if (type === 'success') toast.style.background = '#059669';
    else toast.style.background = '#2563eb';

    document.body.appendChild(toast);
    setTimeout(function() {
      if (toast.parentNode) toast.parentNode.removeChild(toast);
    }, 2600);
  }

  function initTheme() {
    const theme = localStorage.getItem(THEME_KEY) || 'light';
    document.documentElement.setAttribute('data-theme', theme);
  }

  function toggleTheme() {
    const current = document.documentElement.getAttribute('data-theme') || 'light';
    const next = current === 'dark' ? 'light' : 'dark';
    document.documentElement.setAttribute('data-theme', next);
    localStorage.setItem(THEME_KEY, next);
  }

  function getUserId() {
    return localStorage.getItem(USER_ID_KEY) || '';
  }

  function getUserName() {
    return localStorage.getItem(USER_NAME_KEY) || '';
  }

  function getUserRole() {
    return localStorage.getItem(USER_ROLE_KEY) || '';
  }

  function logout() {
    localStorage.removeItem(TOKEN_KEY);
    localStorage.removeItem(USER_ID_KEY);
    localStorage.removeItem(USER_NAME_KEY);
    localStorage.removeItem(USER_ROLE_KEY);
    window.location.href = 'login.html';
  }

  function initAuth(requiredRole) {
    const token = localStorage.getItem(TOKEN_KEY);
    const role = localStorage.getItem(USER_ROLE_KEY);
    if (!token || !role) {
      window.location.href = 'login.html';
      return null;
    }
    if (requiredRole && role !== requiredRole) {
      window.location.href = 'login.html';
      return null;
    }
    if (typeof API !== 'undefined') {
      API.token = token;
    }
    return {
      token: token,
      id: getUserId(),
      name: getUserName(),
      role: role
    };
  }

  function navigateTo(sectionId) {
    const sections = document.querySelectorAll('.page-section');
    const navLinks = document.querySelectorAll('.sidebar-nav [data-section]');
    sections.forEach(function(section) {
      section.classList.remove('active');
    });
    navLinks.forEach(function(link) {
      link.classList.remove('active');
    });

    const activeSection = byId(sectionId);
    if (activeSection) activeSection.classList.add('active');

    const activeLink = document.querySelector('.sidebar-nav [data-section="' + sectionId + '"]');
    if (activeLink) activeLink.classList.add('active');
  }

  function bindSidebarNavigation() {
    const navLinks = document.querySelectorAll('.sidebar-nav [data-section]');
    navLinks.forEach(function(link) {
      link.addEventListener('click', function(event) {
        event.preventDefault();
        navigateTo(this.getAttribute('data-section'));
      });
    });
  }

  function fillForm(form, data) {
    if (!form || !data) return;
    const fields = form.querySelectorAll('[name]');
    fields.forEach(function(field) {
      const name = field.getAttribute('name');
      if (!Object.prototype.hasOwnProperty.call(data, name)) return;
      field.value = data[name] === null || data[name] === undefined ? '' : data[name];
    });
  }

  function formDataObject(form) {
    const result = {};
    if (!form) return result;
    const formData = new FormData(form);
    formData.forEach(function(value, key) {
      result[key] = String(value);
    });
    return result;
  }

  function downloadText(filename, text) {
    const blob = new Blob([text], { type: 'text/plain;charset=utf-8' });
    const url = URL.createObjectURL(blob);
    const anchor = document.createElement('a');
    anchor.href = url;
    anchor.download = filename;
    document.body.appendChild(anchor);
    anchor.click();
    document.body.removeChild(anchor);
    URL.revokeObjectURL(url);
  }

  document.addEventListener('DOMContentLoaded', function() {
    initTheme();
    bindSidebarNavigation();
  });

  window.Common = {
    byId: byId,
    escapeHtml: escapeHtml,
    showToast: showToast,
    initTheme: initTheme,
    toggleTheme: toggleTheme,
    initAuth: initAuth,
    getUserId: getUserId,
    getUserName: getUserName,
    getUserRole: getUserRole,
    logout: logout,
    navigateTo: navigateTo,
    fillForm: fillForm,
    formDataObject: formDataObject,
    downloadText: downloadText
  };

  window.toggleTheme = toggleTheme;
  window.logout = logout;
})();
