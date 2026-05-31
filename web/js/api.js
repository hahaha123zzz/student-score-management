const API = {
    base: 'http://localhost:8080/api',
    token: localStorage.getItem('token') || '',

    async request(method, path, body = null) {
        const headers = { 'Content-Type': 'application/json' };
        if (this.token) headers['Authorization'] = 'Bearer ' + this.token;
        const opts = { method, headers };
        if (body) opts.body = JSON.stringify(body);
        const resp = await fetch(this.base + path, opts);
        return resp.json();
    },

    get(path) { return this.request('GET', path); },
    post(path, data) { return this.request('POST', path, data); },
    put(path, data) { return this.request('PUT', path, data); },
    del(path) { return this.request('DELETE', path); },

    login: (u, p) => API.post('/login', { username: u, password: p }),
    authCheck: () => API.get('/auth/check'),
    changePwd: (oldPwd, newPwd) => API.post('/change-password', { old_password: oldPwd, new_password: newPwd }),

    users: {
        list: () => API.get('/users'),
        add: (d) => API.post('/users', d),
        update: (id, d) => API.put('/users/' + id, d),
        del: (id) => API.del('/users/' + id)
    },

    students: {
        list: (p = {}) => {
            const q = new URLSearchParams();
            if (p.search) q.set('search', p.search);
            if (p.class) q.set('class', p.class);
            if (p.page) q.set('page', p.page);
            if (p.size) q.set('size', p.size || 20);
            return API.get('/students?' + q.toString());
        },
        add: (d) => API.post('/students', d),
        update: (id, d) => API.put('/students/' + id, d),
        del: (id) => API.del('/students/' + id)
    },

    classes: {
        list: () => API.get('/classes'),
        add: (d) => API.post('/classes', d),
        update: (id, d) => API.put('/classes/' + id, d),
        del: (id) => API.del('/classes/' + id)
    },

    subjects: {
        list: () => API.get('/subjects'),
        add: (d) => API.post('/subjects', d)
    },

    exams: {
        list: (s) => API.get('/exams' + (s ? '?status=' + s : '')),
        add: (d) => API.post('/exams', d),
        update: (id, d) => API.put('/exams/' + id, d),
        del: (id) => API.del('/exams/' + id),
        publish: (id) => API.put('/exams/' + id + '/publish'),
        retract: (id) => API.put('/exams/' + id + '/retract')
    },

    grades: {
        set: (d) => API.post('/grades', d),
        import: (csv, examId) => API.post('/grades/import', { csv, exam_id: examId }),
        export: (examId) => API.get('/grades/export?exam_id=' + examId),
        lock: (examId) => API.put('/grades/' + examId + '/lock'),
        list: (examId) => API.get('/grades?exam_id=' + examId),
        student: (sid) => API.get('/grades/student/' + sid),
        makeup: (sid, eid, scores) => API.post('/grades/makeup', { student_id: sid, exam_id: eid, scores })
    },

    stats: {
        rank: (examId, type, subject) => {
            let q = '?exam_id=' + examId + '&type=' + (type || 'grade');
            if (subject) q += '&subject=' + subject;
            return API.get('/stats/rank' + q);
        },
        classCompare: (examId) => API.get('/stats/class-compare?exam_id=' + examId),
        distribution: (examId, subject) => {
            let q = '?exam_id=' + examId;
            if (subject) q += '&subject=' + subject;
            return API.get('/stats/distribution' + q);
        },
        trend: (sid) => API.get('/stats/trend/' + sid),
        warnings: () => API.get('/stats/warnings'),
        enrollment: () => API.get('/stats/enrollment'),
        imbalance: (sid) => API.get('/stats/imbalance?student_id=' + sid),
        ability: (sid, eid) => API.get('/stats/ability?student_id=' + sid + '&exam_id=' + eid)
    },

    report: {
        get: (sid) => API.get('/report?student_id=' + sid)
    },

    suggest: {
        get: (sid) => API.get('/suggest?student_id=' + sid)
    },

    logs: (page = 1, user = '') => API.get('/logs?page=' + page + '&size=50' + (user ? '&user=' + user : '')),

    extras: {
        setError: (d) => API.post('/errors', d),
        getErrors: (p) => {
            const q = new URLSearchParams();
            if (p.student_id) q.set('student_id', p.student_id);
            if (p.exam_id) q.set('exam_id', p.exam_id);
            if (p.subject) q.set('subject', p.subject);
            if (p.page) q.set('page', p.page);
            if (p.size) q.set('size', p.size || 20);
            return API.get('/errors?' + q.toString());
        },
        getErrorStats: (sid) => API.get('/errors/stats?student_id=' + sid)
    },

    goals: {
        set: (data) => API.post('/goals', data),
        get: (sid) => API.get('/goals?student_id=' + sid)
    },

    knowledge: {
        map: (sid) => API.get('/knowledge-map?student_id=' + sid)
    },

    portfolio: {
        get: (sid) => API.get('/portfolio?student_id=' + sid)
    },

    points: {
        get: (sid) => API.get('/points?student_id=' + sid),
        add: (sid, pts, reason) => API.post('/points/add', { student_id: sid, points: pts, reason: reason })
    },

    review: {
        plan: (sid) => API.get('/review-plan?student_id=' + sid)
    },

    checkin: {
        set: (data) => API.post('/checkin', data),
        get: (sid) => API.get('/checkins?student_id=' + sid)
    }
};
