// Frontend API wrapper
// The backend now accepts form-urlencoded bodies instead of JSON.
// This file keeps all request details in one place so the page scripts stay simple.

const API = {
  base: window.location.origin,
  token: localStorage.getItem('token') || '',

  encode(data) {
    if (!data) return '';
    const parts = [];
    Object.keys(data).forEach(function(key) {
      if (data[key] === undefined || data[key] === null) return;
      parts.push(encodeURIComponent(key) + '=' + encodeURIComponent(String(data[key])));
    });
    return parts.join('&');
  },

  async request(method, path, data) {
    const headers = {};
    if (this.token) headers.Authorization = 'Bearer ' + this.token;

    const options = { method: method, headers: headers };
    let url = this.base + path;

    if (method === 'GET' && data && Object.keys(data).length) {
      const query = this.encode(data);
      url += (url.indexOf('?') === -1 ? '?' : '&') + query;
    } else if (data && method !== 'GET') {
      headers['Content-Type'] = 'application/x-www-form-urlencoded; charset=UTF-8';
      options.body = this.encode(data);
    }

    try {
      const response = await fetch(url, options);
      const result = await response.json();
      return result;
    } catch (error) {
      return { success: false, message: '无法连接服务器', data: null };
    }
  },

  get(path, params) { return this.request('GET', path, params); },
  post(path, data) { return this.request('POST', path, data); },
  put(path, data) { return this.request('PUT', path, data); },
  del(path) { return this.request('DELETE', path); },

  login(username, password) {
    return this.post('/api/login', { username: username, password: password });
  },

  authCheck() {
    return this.get('/api/auth/check');
  },

  changePassword(oldPassword, newPassword) {
    return this.post('/api/change-password', {
      old_password: oldPassword,
      new_password: newPassword
    });
  },

  classes: {
    list() {
      return API.get('/api/classes');
    }
  },

  students: {
    list(params) {
      return API.get('/api/students', params || {});
    },
    get(id) {
      return API.get('/api/students/' + encodeURIComponent(id));
    },
    add(data) {
      return API.post('/api/students', data);
    },
    update(id, data) {
      return API.put('/api/students/' + encodeURIComponent(id), data);
    },
    remove(id) {
      return API.del('/api/students/' + encodeURIComponent(id));
    }
  },

  subjects: {
    list() {
      return API.get('/api/subjects');
    },
    add(data) {
      return API.post('/api/subjects', data);
    },
    update(id, data) {
      return API.put('/api/subjects/' + encodeURIComponent(id), data);
    },
    remove(id) {
      return API.del('/api/subjects/' + encodeURIComponent(id));
    }
  },

  exams: {
    list() {
      return API.get('/api/exams');
    },
    add(data) {
      return API.post('/api/exams', data);
    },
    update(id, data) {
      return API.put('/api/exams/' + encodeURIComponent(id), data);
    },
    remove(id) {
      return API.del('/api/exams/' + encodeURIComponent(id));
    }
  },

  grades: {
    list(params) {
      return API.get('/api/grades', params || {});
    },
    student(id) {
      return API.get('/api/grades/student/' + encodeURIComponent(id));
    },
    save(data) {
      return API.post('/api/grades', data);
    },
    remove(examId, studentId) {
      return API.del('/api/grades/' + encodeURIComponent(examId) + '/' + encodeURIComponent(studentId));
    }
  },

  rank: {
    grade(params) {
      return API.get('/api/rank/grade', params || {});
    },
    classRank(params) {
      return API.get('/api/rank/class', params || {});
    }
  },

  stats: {
    overview() {
      return API.get('/api/stats');
    },
    exam(examId) {
      return API.get('/api/stats', { exam_id: examId });
    }
  },

  exportCsv(examId) {
    return API.get('/api/export', { exam_id: examId });
  },

  logs() {
    return API.get('/api/logs');
  }
};
