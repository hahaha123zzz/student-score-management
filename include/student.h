#pragma once
#include "../lib/json.hpp"

using json = nlohmann::json;

namespace student {
    json getStudents(const std::string& search, const std::string& cls, int page, int size);
    json addStudent(const json& data);
    json updateStudent(const std::string& id, const json& data);
    json deleteStudent(const std::string& id);
    json getClasses();
    json addClass(const json& data);
    json updateClass(const std::string& id, const json& data);
    json deleteClass(const std::string& id);
    json getSubjects();
    json addSubject(const json& data);
    json getEnrollmentStats();
}
