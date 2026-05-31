#include "auth.h"
#include "util.h"

namespace auth {

static json sessions = json::object();

json login(const std::string& username, const std::string& password) {
    json users = util::readJSON("users.json");
    for (auto& u : users) {
        if (u["id"].get<std::string>() == username) {
            if (u["password"].get<std::string>() == util::hasher(password)) {
                std::string token = util::genToken();
                sessions[token] = {{"user_id", username}, {"role", u["role"]}, {"name", u["name"]}};
                util::logAction(username, "登录", "用户登录系统");
                return util::jsonResponse(true, "登录成功", {
                    {"token", token},
                    {"role", u["role"]},
                    {"name", u["name"]},
                    {"user_id", username}
                });
            }
            return util::errorResponse("密码错误");
        }
    }
    return util::errorResponse("用户不存在");
}

json checkAuth(const std::string& token) {
    if (token.empty() || !sessions.contains(token))
        return util::errorResponse("未登录或登录已过期");
    return util::jsonResponse(true, "已登录", sessions[token]);
}

json changePassword(const std::string& token, const std::string& oldPwd, const std::string& newPwd) {
    if (!sessions.contains(token))
        return util::errorResponse("请先登录");
    std::string userId = sessions[token]["user_id"];
    json users = util::readJSON("users.json");
    for (auto& u : users) {
        if (u["id"] == userId) {
            if (u["password"] != util::hasher(oldPwd))
                return util::errorResponse("原密码错误");
            if (newPwd.length() < 4)
                return util::errorResponse("新密码至少4位");
            u["password"] = util::hasher(newPwd);
            util::writeJSON("users.json", users);
            util::logAction(userId, "修改密码", "修改登录密码");
            return util::jsonResponse(true, "密码修改成功");
        }
    }
    return util::errorResponse("用户不存在");
}

json getUsers() {
    json users = util::readJSON("users.json");
    for (auto& u : users) u.erase("password");
    return util::jsonResponse(true, "ok", users);
}

json addUser(const json& data) {
    json users = util::readJSON("users.json");
    std::string userId = data.value("id", "");
    if (userId.empty()) return util::errorResponse("用户ID不能为空");
    for (auto& u : users)
        if (u["id"] == userId) return util::errorResponse("用户ID已存在");
    json newUser;
    newUser["id"] = userId;
    newUser["name"] = data.value("name", "");
    newUser["role"] = data.value("role", "student");
    newUser["password"] = util::hasher("123456");
    newUser["phone"] = data.value("phone", "");
    newUser["created_at"] = util::getCurrentTime();
    users.push_back(newUser);
    util::writeJSON("users.json", users);
    util::logAction(userId, "创建用户", "新增用户: " + newUser["name"].get<std::string>());
    return util::jsonResponse(true, "用户创建成功", newUser);
}

json deleteUser(const std::string& userId) {
    json users = util::readJSON("users.json");
    json filtered = json::array();
    bool found = false;
    for (auto& u : users) {
        if (u["id"] == userId) { found = true; continue; }
        filtered.push_back(u);
    }
    if (!found) return util::errorResponse("用户不存在");
    util::writeJSON("users.json", filtered);
    util::logAction(userId, "删除用户", "删除用户: " + userId);
    return util::jsonResponse(true, "用户已删除");
}

json updateUser(const std::string& userId, const json& data) {
    json users = util::readJSON("users.json");
    for (auto& u : users) {
        if (u["id"] == userId) {
            if (data.contains("name")) u["name"] = data["name"];
            if (data.contains("phone")) u["phone"] = data["phone"];
            if (data.contains("role")) u["role"] = data["role"];
            util::writeJSON("users.json", users);
            util::logAction(userId, "更新用户", "更新用户信息: " + userId);
            return util::jsonResponse(true, "用户信息更新成功", u);
        }
    }
    return util::errorResponse("用户不存在");
}

}
