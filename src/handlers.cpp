#include "handlers.h"
#include "models.h"
#include "storage.h"
#include "utils.h"

namespace handlers {

    // ===================================================================
    // 【内部工具】从 JSON 字符串中解析字段值
    // JSON 格式形如 {"key1":"value1", "key2":"value2"}，本函数提取 key 对应的 value
    //
    // 解析策略（分两步）：
    //   ① 先尝试匹配 "key":"value" 格式（字符串值）
    //      → 在 body 中查找 "key":"
    //      → 找到后，从引号后取到下一个引号前的部分即为 value
    //   ② 若没找到，再尝试匹配 "key":value 格式（数字/布尔值）
    //      → 在 body 中查找 "key":
    //      → 找到后，取到最近的逗号或花括号之前的部分，去除首尾空格
    // ===================================================================
    std::string parseBodyField(const std::string& body, const std::string& key) {
        // 构造要搜索的模式串："key":"
        std::string searchKey = "\"" + key + "\":\"";
        size_t pos = body.find(searchKey);               // 在body中查找该模式
        if (pos == std::string::npos) {
            // 没找到字符串格式，尝试数字格式："key":
            searchKey = "\"" + key + "\":";
            pos = body.find(searchKey);
            if (pos == std::string::npos) return "";     // 两种格式都没找到，返回空串
            pos += searchKey.size();                      // 跳过 "key": 部分
        } else {
            // 找到了字符串格式 "key":"value"，提取引号内的内容
            pos += searchKey.size();                      // 跳到 value 的起始位置
            size_t end = body.find("\"", pos);            // 找 value 结束的那个引号
            if (end == std::string::npos) return "";
            return body.substr(pos, end - pos);           // 截取引号之间的部分
        }
        // 处理数字/布尔值格式：找到下一个逗号或右花括号
        size_t end = body.find_first_of(",}", pos);
        if (end == std::string::npos) return "";
        return utils::trim(body.substr(pos, end - pos));   // 截取并去除首尾空格
    }

    // ===== 权限检查 =====

    // 【权限辅助】判断当前令牌对应的用户是否为管理员
    // 先验证 token 是否有效，再查询该用户的角色是否为 "admin"
    bool isAdmin(const std::string& token) {
        if (!utils::isTokenValid(token)) return false;     // token 无效直接返回 false
        std::string role = utils::getUserRoleByToken(token); // 根据 token 查询角色
        return role == "admin";                             // 只有 "admin" 才返回 true
    }

    // ===== 登录 =====

    // ===================================================================
    // 【登录】处理 POST /api/login 请求
    //
    // 输入：body = {"username":"用户ID或姓名", "password":"明文密码"}
    //
    // 验证逻辑（按顺序执行，任一步失败即返回错误）：
    //   ① 解析请求体中的 username 和 password
    //   ② 遍历 users.csv 中的所有用户
    //   ③ 逐行比对：支持用"用户ID"或"姓名"登录（p.getId() 或 p.getName()）
    //   ④ 若找到匹配用户 → 用 bcrypt 验证密码（verifyPassword）
    //   ⑤ 密码正确 → 生成随机 token，将 token 与用户ID 绑定，记录角色
    //   ⑥ 返回 {token, role, name, user_id} 给前端
    //   ⑦ 密码错误 → 返回"密码错误"（不提示用户是否存在，防止枚举攻击）
    //   ⑧ 遍历完都没找到 → 返回"用户不存在"
    //
    // 返回JSON：{"success":true, "data":{"token":"xxx","role":"admin","name":"张三","user_id":"admin001"}}
    // ===================================================================
    std::string login(const std::string& body) {
        std::string username = parseBodyField(body, "username");
        std::string password = parseBodyField(body, "password");

        std::vector<std::vector<std::string>> users = storage::getUsers();
        for (size_t i = 0; i < users.size(); i++) {
            Person p = Person::fromCSV(users[i]);      // 将CSV行转换为Person对象
            // ③ 匹配用户名：允许用ID或姓名登录
            if (p.getId() == username || p.getName() == username) {
                // ④ 验证密码（bcrypt加密比对，非明文比较）
                if (p.verifyPassword(password)) {
                    // ⑤ 密码正确：生成令牌并建立会话
                    std::string token = utils::generateToken();       // 生成随机 token
                    utils::storeToken(token, p.getId());              // 绑定 token 与用户ID
                    utils::setUserRole(p.getId(), p.getRole());       // 记录用户角色
                    // ⑥ 构造返回给前端的 JSON 数据
                    std::string data = "{";
                    data += "\"token\":\"" + utils::jsonEscape(token) + "\",";
                    data += "\"role\":\"" + utils::jsonEscape(p.getRole()) + "\",";
                    data += "\"name\":\"" + utils::jsonEscape(p.getName()) + "\",";
                    data += "\"user_id\":\"" + utils::jsonEscape(username) + "\"";
                    data += "}";
                    utils::logAction(username, "登录", "用户登录系统");  // 记录登录日志
                    return utils::jsonResponse(true, "登录成功", data);
                }
                // ⑦ 密码不匹配，直接返回错误（不透露用户是否存在）
                return utils::errorResponse("密码错误");
            }
        }
        // ⑧ 遍历完所有用户都没匹配上
        return utils::errorResponse("用户不存在");
    }

    // ===================================================================
    // 【修改密码】处理 POST /api/changePassword
    // 输入：token（登录令牌）, body = {"old_password":"旧密码", "new_password":"新密码"}
    //
    // 业务流程：
    //   ① 验证 token 有效性（未登录不能修改）
    //   ② 根据 token 查出当前用户ID
    //   ③ 检查新密码长度 ≥ 4 位
    //   ④ 在用户表中找到该用户，验证旧密码是否正确
    //   ⑤ 旧密码正确 → 对新密码做 bcrypt 哈希后写入 CSV
    //   ⑥ 旧密码错误或用户不存在 → 返回对应错误
    // ===================================================================
    std::string changePassword(const std::string& token, const std::string& body) {
        if (!utils::isTokenValid(token)) return utils::errorResponse("请先登录");
        std::string uid = utils::getUserIdByToken(token);     // 根据 token 反查用户ID
        std::string oldPwd = parseBodyField(body, "old_password");
        std::string newPwd = parseBodyField(body, "new_password");

        if (newPwd.size() < 4) return utils::errorResponse("新密码至少4位");

        // 查询用户表，找到当前用户
        std::vector<std::vector<std::string>> users = storage::getUsers();
        for (size_t i = 0; i < users.size(); i++) {
            Person p = Person::fromCSV(users[i]);
            if (p.getId() == uid) {                           // 找到当前用户
                if (!p.verifyPassword(oldPwd))                // 验证旧密码
                    return utils::errorResponse("原密码错误");
                p.setPassword(utils::hashPassword(newPwd));   // 设置新密码（bcrypt加密）
                users[i] = p.toCSV();                         // 将Person对象转回CSV行
                storage::saveUsers(users);                    // 写回文件
                return utils::jsonResponse(true, "密码修改成功", "{}");
            }
        }
        return utils::errorResponse("用户不存在");
    }

    // ===================================================================
    // 【验证登录状态】处理 GET /api/checkAuth
    // 前端页面加载时调用，检查用户是否处于登录状态
    // 返回当前用户的 user_id 和 role，token 过期则返回"未登录"
    // ===================================================================
    std::string checkAuth(const std::string& token) {
        if (!utils::isTokenValid(token)) return utils::errorResponse("未登录或登录已过期");
        std::string uid = utils::getUserIdByToken(token);
        std::string role = utils::getUserRoleByToken(token);
        // 从 users.csv 中查找用户名
        std::string name = uid;
        std::vector<std::vector<std::string>> users = storage::getUsers();
        for (size_t i = 0; i < users.size(); i++) {
            Person p = Person::fromCSV(users[i]);
            if (p.getId() == uid) { name = p.getName(); break; }
        }
        std::string data = "{\"user_id\":\"" + utils::jsonEscape(uid) + "\",\"role\":\"" + utils::jsonEscape(role) + "\",\"name\":\"" + utils::jsonEscape(name) + "\"}";
        return utils::jsonResponse(true, "已登录", data);
    }

    // ===== 用户管理 =====

    // ===================================================================
    // 【获取所有用户】处理 GET /api/users
    // 从 users.csv 读取全部用户，返回数组格式的 JSON
    // 每个用户对象包含：id（用户ID）、name（姓名）、role（角色）、created_at（创建时间）
    // ===================================================================
    std::string getUsers() {
        std::vector<std::vector<std::string>> users = storage::getUsers();
        std::string data = "[";
        for (size_t i = 0; i < users.size(); i++) {
            if (i > 0) data += ",";                       // 除了第一个元素，前面都加逗号
            Person p = Person::fromCSV(users[i]);
            data += "{";
            data += "\"id\":\"" + utils::jsonEscape(p.getId()) + "\",";
            data += "\"name\":\"" + utils::jsonEscape(p.getName()) + "\",";
            data += "\"role\":\"" + utils::jsonEscape(p.getRole()) + "\",";
            data += "\"created_at\":\"" + utils::jsonEscape(p.getCreatedAt()) + "\"";
            data += "}";
        }
        data += "]";
        return utils::jsonResponse(true, "ok", data);
    }

    // ===================================================================
    // 【添加用户】处理 POST /api/users
    // 输入：body = {"id":"用户ID", "name":"姓名", "role":"admin或student"}
    //
    // 业务流程：
    //   ① 解析 id/name/role 三个字段
    //   ② 如果没提供 role，默认设为 "student"
    //   ③ 检查用户ID是否已存在（遍历 users.csv）
    //   ④ 新建 Person 对象，密码默认为 "123456"（bcrypt加密）
    //   ⑤ 追加到 CSV 并保存
    // ===================================================================
    std::string addUser(const std::string& body) {
        std::string userId = parseBodyField(body, "id");
        std::string name = parseBodyField(body, "name");
        std::string role = parseBodyField(body, "role");
        if (userId.empty()) return utils::errorResponse("用户ID不能为空");
        if (role.empty()) role = "student";               // 默认角色为学生

        std::vector<std::vector<std::string>> users = storage::getUsers();
        for (size_t i = 0; i < users.size(); i++) {
            if (users[i][0] == userId) return utils::errorResponse("用户ID已存在"); // 查重
        }

        // Person构造函数：(用户ID, 姓名, 加密密码, 角色, 创建时间)
        Person p(userId, name, utils::hashPassword("123456"), role, utils::getCurrentTime());
        users.push_back(p.toCSV());                       // 将Person转为CSV行追加
        storage::saveUsers(users);                        // 写回文件
        return utils::jsonResponse(true, "用户创建成功", "{}");
    }

    // ===================================================================
    // 【更新用户】处理 PUT /api/users/{userId}
    // 输入：userId（URL路径参数）, body = {"name":"新姓名", "role":"新角色"}
    // 只更新 body 中提供的字段（非空才覆盖），未提供的字段保持原值不变
    // ===================================================================
    std::string updateUser(const std::string& userId, const std::string& body) {
        std::vector<std::vector<std::string>> users = storage::getUsers();
        for (size_t i = 0; i < users.size(); i++) {
            if (users[i][0] == userId) {                  // 找到匹配的用户
                std::string name = parseBodyField(body, "name");
                std::string role = parseBodyField(body, "role");
                if (!name.empty()) users[i][1] = name;    // 只有提供了才更新
                if (!role.empty()) users[i][3] = role;
                storage::saveUsers(users);
                return utils::jsonResponse(true, "用户信息更新成功", "{}");
            }
        }
        return utils::errorResponse("用户不存在");
    }

    // ===================================================================
    // 【删除用户】处理 DELETE /api/users/{userId}
    // 使用"过滤法"删除：遍历所有用户，保留 ID 不匹配的，达到删除的效果
    // ===================================================================
    std::string deleteUser(const std::string& userId) {
        std::vector<std::vector<std::string>> users = storage::getUsers();
        std::vector<std::vector<std::string>> filtered;   // 新建容器，只保留未被删除的
        for (size_t i = 0; i < users.size(); i++) {
            if (users[i][0] != userId) filtered.push_back(users[i]); // 跳过要删除的那一行
        }
        if (filtered.size() == users.size()) return utils::errorResponse("用户不存在"); // 没删掉任何一行
        storage::saveUsers(filtered);
        return utils::jsonResponse(true, "用户已删除", "{}");
    }

    // ===== 学生管理 =====

    // ===================================================================
    // 【获取学生列表】处理 GET /api/students?search=&class=&page=1&size=20
    // 支持三种高级功能：
    //   search —— 模糊搜索，按"姓名"或"学号"匹配（大小写不敏感）
    //   class  —— 按班级名称精确筛选
    //   page/size —— 分页，page 从 1 开始，每页 size 条
    //
    // 返回：{total, page, size, data: [{id, name, birthday, class, gender}, ...]}
    //
    // 分页算法：
    //   start = (page - 1) * size     // 起始索引（0-based）
    //   end = start + size            // 结束索引（不包含）
    //   只输出 data[start] ~ data[end-1] 段
    // ===================================================================
    std::string getStudents(const std::string& queryString) {
        std::string search = utils::getQueryParam(queryString, "search");
        std::string cls = utils::getQueryParam(queryString, "class");
        std::string pageStr = utils::getQueryParam(queryString, "page");
        std::string sizeStr = utils::getQueryParam(queryString, "size");
        int page = pageStr.empty() ? 1 : std::stoi(pageStr);    // 默认第1页
        int size = sizeStr.empty() ? 20 : std::stoi(sizeStr);   // 默认每页20条

        std::vector<std::vector<std::string>> all = storage::getStudents();
        std::vector<Student> filtered;                           // 用于存放筛选后的结果
        for (size_t i = 0; i < all.size(); i++) {
            Student s = Student::fromCSV(all[i]);
            // 模糊搜索：姓名或学号包含关键词
            if (!search.empty()) {
                std::string lowName = utils::toLower(s.getName());
                std::string lowSearch = utils::toLower(search);
                std::string sid = s.getId();
                if (lowName.find(lowSearch) == std::string::npos && sid.find(search) == std::string::npos)
                    continue;                                    // 两者都不匹配则跳过
            }
            // 按班级筛选
            if (!cls.empty() && s.getClassName() != cls) continue;
            filtered.push_back(s);
        }

        // 分页截取
        int total = (int)filtered.size();
        int start = (page - 1) * size;
        int end = start + size;
        if (end > total) end = total;                            // 防止越界

        // 拼装返回的 JSON（前端期望 d.data 直接是学生数组）
        std::string data = "[";
        for (int i = start; i < end; i++) {
            if (i > start) data += ",";
            data += "{";
            data += "\"id\":\"" + utils::jsonEscape(filtered[i].getId()) + "\",";
            data += "\"name\":\"" + utils::jsonEscape(filtered[i].getName()) + "\",";
            data += "\"birthday\":\"" + utils::jsonEscape(filtered[i].getBirthday()) + "\",";
            data += "\"class\":\"" + utils::jsonEscape(filtered[i].getClassName()) + "\",";
            data += "\"gender\":\"" + utils::jsonEscape(filtered[i].getGender()) + "\"";
            data += "}";
        }
        data += "]";
        return utils::jsonResponse(true, "ok", data);
    }

    // ===================================================================
    // 【添加学生】处理 POST /api/students
    // 输入：body = {"id":"学号","name":"姓名","class":"班级","gender":"性别","birthday":"生日"}
    // 会检查学号是否重复，每行还会自动记录 created_at 创建时间
    // ===================================================================
    std::string addStudent(const std::string& body) {
        std::string sid = parseBodyField(body, "id");
        std::string name = parseBodyField(body, "name");
        std::string className = parseBodyField(body, "class");
        std::string gender = parseBodyField(body, "gender");
        std::string birthday = parseBodyField(body, "birthday");

        if (sid.empty()) return utils::errorResponse("学号不能为空");

        std::vector<std::vector<std::string>> all = storage::getStudents();
        for (size_t i = 0; i < all.size(); i++) {
            if (all[i][0] == sid) return utils::errorResponse("学号已存在"); // 查重
        }
        // CSV列顺序：学号, 姓名, 生日, 班级, 性别, 创建时间
        std::vector<std::string> row;
        row.push_back(sid);
        row.push_back(name);
        row.push_back(birthday);
        row.push_back(className);
        row.push_back(gender);
        row.push_back(utils::getCurrentTime());                 // 自动记录创建时间
        all.push_back(row);
        storage::saveStudents(all);
        return utils::jsonResponse(true, "学生添加成功", "{}");
    }

    // ===================================================================
    // 【更新学生信息】处理 PUT /api/students/{学号}
    // 与用户更新逻辑一致：只更新 body 中提供了的字段（字符串非空），未提供的字段保持原值
    // ===================================================================
    std::string updateStudent(const std::string& id, const std::string& body) {
        std::vector<std::vector<std::string>> all = storage::getStudents();
        for (size_t i = 0; i < all.size(); i++) {
            if (all[i][0] == id) {                              // 按学号定位
                std::string name = parseBodyField(body, "name");
                std::string birthday = parseBodyField(body, "birthday");
                std::string className = parseBodyField(body, "class");
                std::string gender = parseBodyField(body, "gender");
                if (!name.empty()) all[i][1] = name;            // 非空才更新
                if (!birthday.empty()) all[i][2] = birthday;
                if (!className.empty()) all[i][3] = className;
                if (!gender.empty()) all[i][4] = gender;
                storage::saveStudents(all);
                return utils::jsonResponse(true, "学生信息更新成功", "{}");
            }
        }
        return utils::errorResponse("学生不存在");
    }

    // ===================================================================
    // 【删除学生】处理 DELETE /api/students/{学号}
    // 使用"过滤法"——保留所有学号不匹配的行，直接丢弃要删除的那一行
    // 如果过滤前后数量一致，说明没找到该学生
    // ===================================================================
    std::string deleteStudent(const std::string& id) {
        std::vector<std::vector<std::string>> all = storage::getStudents();
        std::vector<std::vector<std::string>> filtered;
        for (size_t i = 0; i < all.size(); i++) {
            if (all[i][0] != id) filtered.push_back(all[i]);   // 过滤掉要删除的行
        }
        if (filtered.size() == all.size()) return utils::errorResponse("学生不存在");
        storage::saveStudents(filtered);
        return utils::jsonResponse(true, "学生已删除", "{}");
    }

    // ===== 班级管理 =====

    // ===================================================================
    // 【获取所有班级】处理 GET /api/classes
    // 返回班级数组，每个班级包含 id, name, grade 三个字段
    // ===================================================================
    std::string getClasses() {
        std::vector<std::vector<std::string>> all = storage::getClasses();
        std::string data = "[";
        for (size_t i = 0; i < all.size(); i++) {
            if (i > 0) data += ",";
            Cls c = Cls::fromCSV(all[i]);
            data += "{";
            data += "\"id\":\"" + utils::jsonEscape(c.getId()) + "\",";
            data += "\"name\":\"" + utils::jsonEscape(c.getName()) + "\",";
            data += "\"grade\":\"" + utils::jsonEscape(c.getGrade()) + "\"";
            data += "}";
        }
        data += "]";
        return utils::jsonResponse(true, "ok", data);
    }

    // ===================================================================
    // 【添加班级】处理 POST /api/classes
    // 输入：body = {"name":"班级名称","grade":"年级"}
    // 班级ID 自动生成，格式为 "CLS" + 序号（如 CLS1, CLS2...）
    // ===================================================================
    std::string addClass(const std::string& body) {
        std::string name = parseBodyField(body, "name");
        std::string grade = parseBodyField(body, "grade");
        if (name.empty()) return utils::errorResponse("班级名称不能为空");

        std::vector<std::vector<std::string>> all = storage::getClasses();
        for (size_t i = 0; i < all.size(); i++) {
            if (all[i][1] == name) return utils::errorResponse("班级已存在"); // 通过名称查重
        }

        std::string newId = "CLS" + std::to_string(all.size() + 1); // 自动生成ID
        // CSV列顺序：ID, 名称, 年级, 创建时间
        std::vector<std::string> row;
        row.push_back(newId);
        row.push_back(name);
        row.push_back(grade);
        row.push_back(utils::getCurrentTime());
        all.push_back(row);
        storage::saveClasses(all);
        return utils::jsonResponse(true, "班级添加成功", "{}");
    }

    // ===================================================================
    // 【更新班级】处理 PUT /api/classes/{班级ID}
    // 只更新 body 中提供的字段（name 或 grade）
    // ===================================================================
    std::string updateClass(const std::string& id, const std::string& body) {
        std::vector<std::vector<std::string>> all = storage::getClasses();
        for (size_t i = 0; i < all.size(); i++) {
            if (all[i][0] == id) {
                std::string name = parseBodyField(body, "name");
                std::string grade = parseBodyField(body, "grade");
                if (!name.empty()) all[i][1] = name;
                if (!grade.empty()) all[i][2] = grade;
                storage::saveClasses(all);
                return utils::jsonResponse(true, "班级更新成功", "{}");
            }
        }
        return utils::errorResponse("班级不存在");
    }

    // ===================================================================
    // 【删除班级】处理 DELETE /api/classes/{班级ID}
    // ===================================================================
    std::string deleteClass(const std::string& id) {
        std::vector<std::vector<std::string>> all = storage::getClasses();
        std::vector<std::vector<std::string>> filtered;
        for (size_t i = 0; i < all.size(); i++) {
            if (all[i][0] != id) filtered.push_back(all[i]);
        }
        if (filtered.size() == all.size()) return utils::errorResponse("班级不存在");
        storage::saveClasses(filtered);
        return utils::jsonResponse(true, "班级已删除", "{}");
    }

    // ===== 科目管理 =====

    // ===================================================================
    // 【获取所有科目】处理 GET /api/subjects
    // 返回科目数组：[{id, name, full_score}, ...]
    // full_score 为满分值，默认 100，可自定义（如语文150分）
    // ===================================================================
    std::string getSubjects() {
        std::vector<std::vector<std::string>> all = storage::getSubjects();
        std::string data = "[";
        for (size_t i = 0; i < all.size(); i++) {
            if (i > 0) data += ",";
            Subject s = Subject::fromCSV(all[i]);
            data += "{";
            data += "\"id\":\"" + utils::jsonEscape(s.getId()) + "\",";
            data += "\"name\":\"" + utils::jsonEscape(s.getName()) + "\",";
            data += "\"full_score\":" + std::to_string(s.getFullScore());
            data += "}";
        }
        data += "]";
        return utils::jsonResponse(true, "ok", data);
    }

    // ===================================================================
    // 【添加科目】处理 POST /api/subjects
    // 输入：body = {"name":"科目名称","full_score":150}
    // 若未提供满分值，默认取 100 分
    // 科目ID 自动生成，格式 "SUB" + 序号
    // ===================================================================
    std::string addSubject(const std::string& body) {
        std::string name = parseBodyField(body, "name");
        std::string fullScoreStr = parseBodyField(body, "full_score");
        int fullScore = fullScoreStr.empty() ? 100 : std::stoi(fullScoreStr); // 默认满分100
        if (name.empty()) return utils::errorResponse("科目名称不能为空");

        std::vector<std::vector<std::string>> all = storage::getSubjects();
        for (size_t i = 0; i < all.size(); i++) {
            if (all[i][1] == name) return utils::errorResponse("科目已存在");
        }

        std::string newId = "SUB" + std::to_string(all.size() + 1);
        // CSV列顺序：ID, 名称, 满分
        std::vector<std::string> row;
        row.push_back(newId);
        row.push_back(name);
        row.push_back(std::to_string(fullScore));
        all.push_back(row);
        storage::saveSubjects(all);
        return utils::jsonResponse(true, "科目添加成功", "{}");
    }

    // ===== 考试管理 =====

    // ===================================================================
    // 【获取考试列表】处理 GET /api/exams?status=draft/published/locked
    //
    // 返回每个考试的完整信息，包括：
    //   - 基本字段：id, name, date, status
    //   - subjects 数组：该考试包含的所有科目
    //   - weight_config 对象：每科的满分值和权重
    //     格式：{"语文":{"full":150,"weight":1.0}, "数学":{"full":150,"weight":1.0}}
    //
    // 可选参数 status 用于过滤（草稿/已发布/已锁定）
    // ===================================================================
    std::string getExams(const std::string& queryString) {
        std::string status = utils::getQueryParam(queryString, "status");
        std::vector<std::vector<std::string>> all = storage::getExams();
        std::string data = "[";
        bool first = true;
        for (size_t i = 0; i < all.size(); i++) {
            Exam e = Exam::fromCSV(all[i]);
            if (!status.empty() && e.getStatus() != status) continue; // 状态过滤
            if (!first) data += ","; first = false;
            data += "{";
            data += "\"id\":\"" + utils::jsonEscape(e.getId()) + "\",";
            data += "\"name\":\"" + utils::jsonEscape(e.getName()) + "\",";
            data += "\"date\":\"" + utils::jsonEscape(e.getDate()) + "\",";
            data += "\"status\":\"" + utils::jsonEscape(e.getStatus()) + "\",";
            data += "\"subjects\":[";
            // subjects 以竖线分隔存储，如 "语文|数学|英语"
            std::vector<std::string> slist = e.getSubjectList();
            for (size_t j = 0; j < slist.size(); j++) {
                if (j > 0) data += ",";
                data += "\"" + utils::jsonEscape(slist[j]) + "\"";
            }
            data += "],";
            data += "\"weight_config\":{";
            // weight_config 以竖线分隔，格式为 "科目名:满分:权重|科目名:满分:权重"
            std::vector<std::string> configs = utils::split(e.getWeightConfig(), '|');
            for (size_t j = 0; j < configs.size(); j++) {
                std::vector<std::string> parts = utils::split(configs[j], ':'); // 用冒号拆出三部分
                if (parts.size() >= 3) {
                    if (j > 0) data += ",";
                    data += "\"" + utils::jsonEscape(parts[0]) + "\":{";
                    data += "\"full\":" + parts[1] + ",";     // 满分值
                    data += "\"weight\":" + parts[2];          // 权重系数
                    data += "}";
                }
            }
            data += "}";
            data += "}";
        }
        data += "]";
        return utils::jsonResponse(true, "ok", data);
    }

    // ===================================================================
    // 【创建考试】处理 POST /api/exams
    // 输入：body = {"name":"期中考试","date":"2024-06-15"}
    // 默认包含语文、数学、英语三科，满分各150，权重各1.0
    // 初始状态为 "draft"（草稿），学生端不可见
    // ===================================================================
    std::string addExam(const std::string& body) {
        std::string name = parseBodyField(body, "name");
        std::string date = parseBodyField(body, "date");
        if (name.empty()) return utils::errorResponse("考试名称不能为空");

        std::vector<std::vector<std::string>> all = storage::getExams();
        std::string newId = "EXAM" + std::to_string(all.size() + 1);

        // 默认科目和权重配置
        std::string subjectsStr = "语文|数学|英语";               // 竖线分隔的科目列表
        std::string weightConfig = "语文:150:1.0|数学:150:1.0|英语:150:1.0"; // 科目:满分:权重

        // CSV列顺序：ID, 名称, 日期, 状态, 科目列表, 权重配置
        std::vector<std::string> row;
        row.push_back(newId);
        row.push_back(name);
        row.push_back(date);
        row.push_back("draft");                                  // 初始状态为草稿
        row.push_back(subjectsStr);
        row.push_back(weightConfig);
        all.push_back(row);
        storage::saveExams(all);
        return utils::jsonResponse(true, "考试创建成功", "{}");
    }

    // ===================================================================
    // 【更新考试】处理 PUT /api/exams/{考试ID}
    // 只更新 body 中提供的字段（name 或 date）
    // ===================================================================
    std::string updateExam(const std::string& id, const std::string& body) {
        std::vector<std::vector<std::string>> all = storage::getExams();
        for (size_t i = 0; i < all.size(); i++) {
            if (all[i][0] == id) {
                std::string name = parseBodyField(body, "name");
                std::string date = parseBodyField(body, "date");
                if (!name.empty()) all[i][1] = name;
                if (!date.empty()) all[i][2] = date;
                storage::saveExams(all);
                return utils::jsonResponse(true, "考试更新成功", "{}");
            }
        }
        return utils::errorResponse("考试不存在");
    }

    // ===================================================================
    // 【删除考试】处理 DELETE /api/exams/{考试ID}
    // ===================================================================
    std::string deleteExam(const std::string& id) {
        std::vector<std::vector<std::string>> all = storage::getExams();
        std::vector<std::vector<std::string>> filtered;
        for (size_t i = 0; i < all.size(); i++) {
            if (all[i][0] != id) filtered.push_back(all[i]);
        }
        if (filtered.size() == all.size()) return utils::errorResponse("考试不存在");
        storage::saveExams(filtered);
        return utils::jsonResponse(true, "考试已删除", "{}");
    }

    // ===================================================================
    // 【发布成绩】处理 PUT /api/exams/{考试ID}/publish
    // 将考试状态从 "draft" 改为 "published"
    //
    // ★重要：状态机逻辑 —— 考试有三种状态
    //   draft     → 草稿，学生端不可见，管理员可编辑
    //   published → 已发布，学生可查看成绩
    //   locked    → 已锁定，任何人不可修改，也不能发布/撤回
    //
    // 已锁定的考试不能再发布，这是数据保护机制：
    //   locked 状态是最终状态，一旦锁定就"封存"了
    // ===================================================================
    std::string publishExam(const std::string& id) {
        std::vector<std::vector<std::string>> all = storage::getExams();
        for (size_t i = 0; i < all.size(); i++) {
            if (all[i][0] == id) {
                if (all[i][3] == "locked") return utils::errorResponse("已锁定不能发布"); // 锁定状态不允许发布
                all[i][3] = "published";                       // 修改第4列（索引3）的状态字段
                storage::saveExams(all);
                return utils::jsonResponse(true, "成绩已发布", "{}");
            }
        }
        return utils::errorResponse("考试不存在");
    }

    // ===================================================================
    // 【撤回成绩】处理 PUT /api/exams/{考试ID}/retract
    // 将考试状态从 "published" 改回 "draft"
    // 撤回后学生端不再可见，管理员可以继续编辑成绩
    // 同样，locked 状态不可撤回
    // ===================================================================
    std::string retractExam(const std::string& id) {
        std::vector<std::vector<std::string>> all = storage::getExams();
        for (size_t i = 0; i < all.size(); i++) {
            if (all[i][0] == id) {
                if (all[i][3] == "locked") return utils::errorResponse("已锁定不能撤回");
                all[i][3] = "draft";                          // 改回草稿状态
                storage::saveExams(all);
                return utils::jsonResponse(true, "成绩已撤回", "{}");
            }
        }
        return utils::errorResponse("考试不存在");
    }

    // ===== 成绩管理 =====

    // ===================================================================
    // 【录入/修改成绩】处理 POST /api/grades
    // 输入：body = {"exam_id":"考试ID", "student_id":"学号", "scores":"85,92,78"}
    //
    // ★★★ 覆盖逻辑详解（这是成绩管理的核心）★★★
    //   scores 字段是一个用逗号分隔的字符串，如 "85,92,78"
    //   对应考试中每个科目的分数（按科目顺序排列）
    //
    //   录入流程：
    //     ① 解析 body 中的 exam_id, student_id, scores
    //     ② 在 grades.csv 中查找 学号+考试ID 同时匹配的记录
    //     ③ 如果找到了 → 直接覆盖该行的 scores 列（修改现有成绩）
    //     ④ 如果没找到 → 新增一行成绩记录（首次录入）
    //
    //   本质是"存在即更新，不存在即插入"（即数据库中的 UPSERT 操作）
    //   这意味着重复提交同一个学生的成绩不会产生多条记录，总是保持一科一考一条
    // ===================================================================
    std::string setGrades(const std::string& body) {
        std::string examId = parseBodyField(body, "exam_id");
        std::string studentId = parseBodyField(body, "student_id");
        if (examId.empty() || studentId.empty())
            return utils::errorResponse("考试ID和学生ID不能为空");

        // 手动提取 scores JSON 对象 {"语文":85,"数学":92} → 转为 "85|92" 存储
        std::string scores;
        size_t scoresPos = body.find("\"scores\":{");
        if (scoresPos != std::string::npos) {
            scoresPos += 10; // 跳过 "scores":{
            // 用括号计数找到配对的 }
            int braceCount = 1;
            size_t j = scoresPos;
            while (j < body.size() && braceCount > 0) {
                if (body[j] == '{') braceCount++;
                else if (body[j] == '}') braceCount--;
                j++;
            }
            std::string scoresObj = body.substr(scoresPos, j - scoresPos - 1); // 去掉末尾的 }
            // 解析 key:value 对（忽略 key，只取 value）
            size_t k = 0;
            bool firstVal = true;
            while (k < scoresObj.size()) {
                // 找冒号
                size_t colon = scoresObj.find(':', k);
                if (colon == std::string::npos) break;
                k = colon + 1;
                // 跳过空白
                while (k < scoresObj.size() && (scoresObj[k] == ' ' || scoresObj[k] == '\t')) k++;
                // 读取数字值
                size_t start = k;
                while (k < scoresObj.size() && scoresObj[k] != ',' && scoresObj[k] != '}') k++;
                std::string val = scoresObj.substr(start, k - start);
                if (!firstVal) scores += "|";
                firstVal = false;
                scores += val;
                if (k < scoresObj.size() && scoresObj[k] == ',') k++;
            }
        }

        std::vector<std::vector<std::string>> grades = storage::getGrades();
        bool found = false;
        for (size_t i = 0; i < grades.size(); i++) {
            if (grades[i][1] == studentId && grades[i][2] == examId) {
                grades[i][3] = scores;
                found = true;
                break;
            }
        }
        if (!found) {
            std::vector<std::string> row;
            row.push_back(std::to_string(grades.size() + 1));
            row.push_back(studentId);
            row.push_back(examId);
            row.push_back(scores);
            row.push_back("false");
            row.push_back("admin");
            row.push_back(utils::getCurrentTime());
            grades.push_back(row);
        }
        storage::saveGrades(grades);
        return utils::jsonResponse(true, "成绩保存成功", "{}");
    }

    // ===================================================================
    // 【CSV导入成绩】处理 POST /api/grades/import
    // 当前为预留接口，返回"开发中"提示
    // ===================================================================
    std::string importCSV(const std::string& body) {
        return utils::errorResponse("CSV导入功能开发中");
    }

    // ===================================================================
    // 【CSV导出成绩】处理 GET /api/grades/export?exam_id=考试ID
    //
    // 业务流程：
    //   ① 根据 exam_id 找到考试信息，取出科目列表
    //   ② 生成 CSV 表头：学号,姓名,科目1,科目2,...
    //   ③ 遍历成绩表，筛选该考试的成绩记录
    //   ④ 通过学号关联学生表，取出学生姓名
    //   ⑤ 将 scores 字段（逗号分隔）拆分为各科分数，填入对应列
    //
    // 返回 JSON：{"success":true, "data":{"csv":"学号,姓名,...\\r\\n001,张三,..."}}
    // 前端拿到 csv 字符串后可保存为 .csv 文件下载
    // ===================================================================
    std::string exportCSV(const std::string& queryString) {
        std::string examId = utils::getQueryParam(queryString, "exam_id");
        if (examId.empty()) return utils::errorResponse("请指定考试ID");

        // ① 找到考试，获取科目列表
        std::vector<std::vector<std::string>> exams = storage::getExams();
        std::string subjects;
        for (size_t i = 0; i < exams.size(); i++) {
            if (exams[i][0] == examId) { subjects = exams[i][4]; break; }
        }
        if (subjects.empty()) return utils::errorResponse("考试不存在");

        std::vector<std::vector<std::string>> grades = storage::getGrades();
        std::vector<std::vector<std::string>> students = storage::getStudents();

        std::vector<std::string> subList = utils::split(subjects, '|'); // 拆分科目列
        // ② 构造CSV表头行
        std::string csv = "学号,姓名";
        for (size_t i = 0; i < subList.size(); i++) csv += "," + subList[i];
        csv += "\r\n";                                        // Windows换行符

        // ③④⑤ 遍历成绩，逐行输出
        for (size_t i = 0; i < grades.size(); i++) {
            if (grades[i][2] != examId) continue;             // 只处理该考试的成绩
            std::string sid = grades[i][1];                   // 学号
            csv += sid + ",";
            // ④ 查找学生姓名
            std::string sname = sid;
            for (size_t j = 0; j < students.size(); j++) {
                if (students[j][0] == sid) { sname = students[j][1]; break; }
            }
            csv += sname;

            // ⑤ 拆分scores填入各科目列
            Grade g = Grade::fromCSV(grades[i]);
            std::vector<double> scoreList = g.getScoreList();  // 如 [85, 92, 78]
            for (size_t j = 0; j < subList.size(); j++) {
                csv += ",";
                if (j < scoreList.size())
                    csv += std::to_string((int)scoreList[j]);   // 转为整数显示
            }
            csv += "\r\n";
        }

        std::string data = "{\"csv\":\"" + utils::jsonEscape(csv) + "\"}";
        return utils::jsonResponse(true, "ok", data);
    }

    // ===================================================================
    // 【锁定成绩】处理 POST /api/grades/lock/{考试ID}
    // 将考试状态改为 "locked"，此后该考试的成绩不可再修改、发布或撤回
    // 用于期末成绩归档等场景
    // ===================================================================
    std::string lockGrades(const std::string& examId) {
        std::vector<std::vector<std::string>> all = storage::getExams();
        for (size_t i = 0; i < all.size(); i++) {
            if (all[i][0] == examId) {
                all[i][3] = "locked";                         // 最终状态
                storage::saveExams(all);
                return utils::jsonResponse(true, "成绩已锁定", "{}");
            }
        }
        return utils::errorResponse("考试不存在");
    }

    // ===================================================================
    // 【查询成绩列表】处理 GET /api/grades?exam_id=考试ID&student_id=学号（可选）
    //
    // 参数均为可选：
    //   exam_id    —— 按考试筛选
    //   student_id —— 按学号筛选
    //   两个参数都不传时，返回所有成绩
    //
    // 返回的每条成绩还关联了学生姓名和班级信息（通过学号关联 students 表查询）
    // 每条成绩对象包含：student_id, exam_id, student_name, class, scores, is_makeup
    // ===================================================================
    std::string getGrades(const std::string& queryString) {
        std::string examId = utils::getQueryParam(queryString, "exam_id");
        std::string studentId = utils::getQueryParam(queryString, "student_id");

        std::vector<std::vector<std::string>> grades = storage::getGrades();
        std::vector<std::vector<std::string>> students = storage::getStudents();
        std::vector<std::vector<std::string>> exams = storage::getExams();

        std::string data = "[";
        bool first = true;
        for (size_t i = 0; i < grades.size(); i++) {
            if (!examId.empty() && grades[i][2] != examId) continue;
            if (!studentId.empty() && grades[i][1] != studentId) continue;
            if (!first) data += ","; first = false;

            std::string sname, sclass;
            for (size_t j = 0; j < students.size(); j++) {
                if (students[j][0] == grades[i][1]) {
                    sname = students[j][1];
                    sclass = students[j][3];
                    break;
                }
            }

            // 查找考试以获取科目列表
            std::vector<std::string> subList;
            for (size_t j = 0; j < exams.size(); j++) {
                if (exams[j][0] == grades[i][2]) {
                    subList = utils::split(exams[j][4], '|');
                    break;
                }
            }

            // 展开 scores: "85|92|78" → {"语文":85,"数学":92,"英语":78}
            std::vector<double> scoreList = Grade::fromCSV(grades[i]).getScoreList();
            std::string scoresObj = "{";
            for (size_t j = 0; j < scoreList.size() && j < subList.size(); j++) {
                if (j > 0) scoresObj += ",";
                scoresObj += "\"" + utils::jsonEscape(subList[j]) + "\":" + std::to_string(scoreList[j]);
            }
            scoresObj += "}";

            data += "{";
            data += "\"student_id\":\"" + utils::jsonEscape(grades[i][1]) + "\",";
            data += "\"exam_id\":\"" + utils::jsonEscape(grades[i][2]) + "\",";
            data += "\"student_name\":\"" + utils::jsonEscape(sname) + "\",";
            data += "\"class\":\"" + utils::jsonEscape(sclass) + "\",";
            data += "\"scores\":" + scoresObj + ",";
            data += "\"is_makeup\":" + std::string(grades[i][4] == "true" ? "true" : "false");
            data += "}";
        }
        data += "]";
        return utils::jsonResponse(true, "ok", data);
    }

    // ===================================================================
    // 【查询学生所有成绩】处理 GET /api/grades/{学号}
    // 内部直接调用 getGrades("student_id=学号")，复用成绩查询逻辑
    // ===================================================================
    std::string getStudentGrades(const std::string& studentId) {
        return getGrades("student_id=" + studentId);
    }

    // ===================================================================
    // 【录入补考成绩】处理 POST /api/grades/makeup
    // 输入：body = {"student_id":"学号", "exam_id":"考试ID", "scores":"补考分数,..."}
    //
    // 与普通 setGrades 的区别：
    //   ① 补考成绩必须基于已有的成绩记录（学生必须先有正常考试成绩才能录入补考）
    //   ② 录入后 is_makeup 字段标记为 "true"，前端可据此显示"补考"标签
    //   ③ 如果找不到原有成绩记录，返回错误"未找到该学生的成绩记录"
    //   ④ 补考的 scores 直接覆盖原有成绩
    // ===================================================================
    std::string markMakeup(const std::string& body) {
        std::string studentId = parseBodyField(body, "student_id");
        std::string examId = parseBodyField(body, "exam_id");
        std::string scores = parseBodyField(body, "scores");

        std::vector<std::vector<std::string>> grades = storage::getGrades();
        bool found = false;
        for (size_t i = 0; i < grades.size(); i++) {
            if (grades[i][1] == studentId && grades[i][2] == examId) {
                grades[i][3] = scores;                       // 覆盖为补考成绩
                grades[i][4] = "true";                       // 标记为补考
                found = true;
                break;
            }
        }
        if (!found) return utils::errorResponse("未找到该学生的成绩记录"); // 必须有原始记录
        storage::saveGrades(grades);
        return utils::jsonResponse(true, "补考成绩已录入", "{}");
    }

    // ===== 日志 =====

    // ===================================================================
    // 【获取操作日志】处理 GET /api/logs?page=1
    // 默认每页50条，按时间倒序排列
    // ===================================================================
    std::string getLogs(const std::string& queryString) {
        std::string pageStr = utils::getQueryParam(queryString, "page");
        int page = pageStr.empty() ? 1 : std::stoi(pageStr); // 默认第1页
        int size = 50;                                       // 每页固定50条
        return utils::getLogs(page, size, "");
    }

}
