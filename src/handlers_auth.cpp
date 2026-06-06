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
        return role == "admin" || role == "teacher";        // admin 和 teacher 都有管理权限
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
