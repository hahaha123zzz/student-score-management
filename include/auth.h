#pragma once
#include "../lib/json.hpp"

using json = nlohmann::json;

namespace auth {
    json login(const std::string& username, const std::string& password);
    json changePassword(const std::string& token, const std::string& oldPwd, const std::string& newPwd);
    json checkAuth(const std::string& token);
    json getUsers();
    json addUser(const json& userData);
    json deleteUser(const std::string& userId);
    json updateUser(const std::string& userId, const json& data);
}
