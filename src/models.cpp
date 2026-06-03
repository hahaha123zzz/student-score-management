#include "models.h"
#include "utils.h"
#include <cstdlib>

// ===== Person =====

Person::Person() : m_id(""), m_name(""), m_password(""), m_role("student"), m_createdAt("") {}

Person::Person(const std::string& id, const std::string& name, const std::string& password,
               const std::string& role, const std::string& createdAt)
    : m_id(id), m_name(name), m_password(password), m_role(role), m_createdAt(createdAt) {}

std::string Person::getId() const { return m_id; }
std::string Person::getName() const { return m_name; }
std::string Person::getPassword() const { return m_password; }
std::string Person::getRole() const { return m_role; }
std::string Person::getCreatedAt() const { return m_createdAt; }

void Person::setName(const std::string& name) { m_name = name; }
void Person::setPassword(const std::string& password) { m_password = password; }
void Person::setRole(const std::string& role) { m_role = role; }

bool Person::verifyPassword(const std::string& input) const {
    return utils::hashPassword(input) == m_password;
}

std::vector<std::string> Person::toCSV() const {
    std::vector<std::string> row;
    row.push_back(m_id);
    row.push_back(m_name);
    row.push_back(m_password);
    row.push_back(m_role);
    row.push_back("");
    row.push_back(m_createdAt);
    return row;
}

Person Person::fromCSV(const std::vector<std::string>& fields) {
    if (fields.size() < 4) return Person();
    return Person(fields[0], fields[1], fields[2], fields[3], fields.size() > 5 ? fields[5] : "");
}

// ===== Admin =====

Admin::Admin() : Person() { m_role = "admin"; }
Admin::Admin(const std::string& id, const std::string& name, const std::string& password,
             const std::string& createdAt)
    : Person(id, name, password, "admin", createdAt) {}

// ===== Student =====

Student::Student() : Person(), m_className(""), m_birthday(""), m_gender(""), m_enrolledAt("") {}

Student::Student(const std::vector<std::string>& fields) {
    if (fields.size() < 6) return;
    m_id = fields[0];
    m_name = fields[1];
    m_birthday = fields[2];
    m_className = fields[3];
    m_gender = fields[4];
    m_enrolledAt = fields[5];
    m_role = "student";
    m_password = "";
}

std::string Student::getClassName() const { return m_className; }
std::string Student::getBirthday() const { return m_birthday; }
std::string Student::getGender() const { return m_gender; }

void Student::setClassName(const std::string& cls) { m_className = cls; }
void Student::setBirthday(const std::string& birthday) { m_birthday = birthday; }
void Student::setGender(const std::string& gender) { m_gender = gender; }

std::vector<std::string> Student::toCSV() const {
    std::vector<std::string> row;
    row.push_back(m_id);
    row.push_back(m_name);
    row.push_back(m_birthday);
    row.push_back(m_className);
    row.push_back(m_gender);
    row.push_back(m_enrolledAt);
    return row;
}

Student Student::fromCSV(const std::vector<std::string>& fields) {
    return Student(fields);
}

// ===== Cls =====

Cls::Cls() : m_id(""), m_name(""), m_grade(""), m_createdAt("") {}

Cls::Cls(const std::string& id, const std::string& name, const std::string& grade, const std::string& createdAt)
    : m_id(id), m_name(name), m_grade(grade), m_createdAt(createdAt) {}

std::string Cls::getId() const { return m_id; }
std::string Cls::getName() const { return m_name; }
std::string Cls::getGrade() const { return m_grade; }

void Cls::setName(const std::string& name) { m_name = name; }
void Cls::setGrade(const std::string& grade) { m_grade = grade; }

std::vector<std::string> Cls::toCSV() const {
    std::vector<std::string> row;
    row.push_back(m_id);
    row.push_back(m_name);
    row.push_back(m_grade);
    row.push_back(m_createdAt);
    return row;
}

Cls Cls::fromCSV(const std::vector<std::string>& fields) {
    if (fields.size() < 4) return Cls();
    return Cls(fields[0], fields[1], fields[2], fields[3]);
}

// ===== Subject =====

Subject::Subject() : m_id(""), m_name(""), m_fullScore(100) {}

Subject::Subject(const std::string& id, const std::string& name, int fullScore)
    : m_id(id), m_name(name), m_fullScore(fullScore) {}

std::string Subject::getId() const { return m_id; }
std::string Subject::getName() const { return m_name; }
int Subject::getFullScore() const { return m_fullScore; }

std::vector<std::string> Subject::toCSV() const {
    std::vector<std::string> row;
    row.push_back(m_id);
    row.push_back(m_name);
    row.push_back(std::to_string(m_fullScore));
    return row;
}

Subject Subject::fromCSV(const std::vector<std::string>& fields) {
    if (fields.size() < 3) return Subject();
    return Subject(fields[0], fields[1], std::stoi(fields[2]));
}

// ===== Exam =====

Exam::Exam() : m_id(""), m_name(""), m_date(""), m_status("draft"), m_subjects(""), m_weightConfig("") {}

Exam::Exam(const std::string& id, const std::string& name, const std::string& date,
           const std::string& status, const std::string& subjects, const std::string& weightConfig)
    : m_id(id), m_name(name), m_date(date), m_status(status),
      m_subjects(subjects), m_weightConfig(weightConfig) {}

std::string Exam::getId() const { return m_id; }
std::string Exam::getName() const { return m_name; }
std::string Exam::getDate() const { return m_date; }
std::string Exam::getStatus() const { return m_status; }
std::string Exam::getSubjects() const { return m_subjects; }
std::string Exam::getWeightConfig() const { return m_weightConfig; }

std::vector<std::string> Exam::getSubjectList() const {
    return utils::split(m_subjects, '|');
}

void Exam::setName(const std::string& name) { m_name = name; }
void Exam::setDate(const std::string& date) { m_date = date; }
void Exam::setStatus(const std::string& status) { m_status = status; }
void Exam::setSubjects(const std::string& subjects) { m_subjects = subjects; }
void Exam::setWeightConfig(const std::string& wc) { m_weightConfig = wc; }

bool Exam::publish() {
    if (m_status == "locked") return false;
    m_status = "published";
    return true;
}
bool Exam::retract() {
    if (m_status == "locked") return false;
    m_status = "draft";
    return true;
}
bool Exam::lock() {
    m_status = "locked";
    return true;
}

double Exam::getSubjectWeight(const std::string& subject) const {
    std::vector<std::string> configs = utils::split(m_weightConfig, '|');
    for (size_t i = 0; i < configs.size(); i++) {
        std::vector<std::string> parts = utils::split(configs[i], ':');
        if (parts.size() >= 3 && parts[0] == subject)
            return std::stod(parts[2]);
    }
    return 1.0;
}

double Exam::getSubjectFull(const std::string& subject) const {
    std::vector<std::string> configs = utils::split(m_weightConfig, '|');
    for (size_t i = 0; i < configs.size(); i++) {
        std::vector<std::string> parts = utils::split(configs[i], ':');
        if (parts.size() >= 3 && parts[0] == subject)
            return std::stod(parts[1]);
    }
    return 100.0;
}

std::vector<std::string> Exam::toCSV() const {
    std::vector<std::string> row;
    row.push_back(m_id);
    row.push_back(m_name);
    row.push_back(m_date);
    row.push_back(m_status);
    row.push_back(m_subjects);
    row.push_back(m_weightConfig);
    return row;
}

Exam Exam::fromCSV(const std::vector<std::string>& fields) {
    if (fields.size() < 6) return Exam();
    return Exam(fields[0], fields[1], fields[2], fields[3], fields[4], fields[5]);
}

// ===== Grade =====

Grade::Grade() : m_id(""), m_studentId(""), m_examId(""), m_scores(""), m_isMakeup(false),
                  m_submittedBy(""), m_submittedAt("") {}

Grade::Grade(const std::vector<std::string>& fields) {
    if (fields.size() < 7) return;
    m_id = fields[0];
    m_studentId = fields[1];
    m_examId = fields[2];
    m_scores = fields[3];
    m_isMakeup = (fields[4] == "true" || fields[4] == "1");
    m_submittedBy = fields[5];
    m_submittedAt = fields[6];
}

std::string Grade::getRowId() const { return m_id; }
std::string Grade::getStudentId() const { return m_studentId; }
std::string Grade::getExamId() const { return m_examId; }
std::string Grade::getScores() const { return m_scores; }
bool Grade::getIsMakeup() const { return m_isMakeup; }

double Grade::getScoreByIndex(int index) const {
    std::vector<double> list = getScoreList();
    if (index >= 0 && index < (int)list.size()) return list[index];
    return 0;
}

std::vector<double> Grade::getScoreList() const {
    std::vector<double> result;
    std::vector<std::string> parts = utils::split(m_scores, '|');
    for (size_t i = 0; i < parts.size(); i++) {
        try {
            result.push_back(std::stod(parts[i]));
        } catch (...) {
            result.push_back(0);
        }
    }
    return result;
}

void Grade::setScores(const std::string& scores) { m_scores = scores; }
void Grade::setIsMakeup(bool makeup) { m_isMakeup = makeup; }

double Grade::calcTotal(const std::string& weightConfig) const {
    std::vector<double> scores = getScoreList();
    std::vector<std::string> configs = utils::split(weightConfig, '|');
    double totalWeighted = 0, totalWeight = 0;
    for (size_t i = 0; i < scores.size() && i < configs.size(); i++) {
        std::vector<std::string> parts = utils::split(configs[i], ':');
        double full = (parts.size() >= 2 ? std::stod(parts[1]) : 100.0);
        double weight = (parts.size() >= 3 ? std::stod(parts[2]) : 1.0);
        totalWeighted += (scores[i] / full) * weight;
        totalWeight += weight;
    }
    return totalWeight > 0 ? (totalWeighted / totalWeight) * 100 : 0;
}

double Grade::calcAverage() const {
    std::vector<double> list = getScoreList();
    if (list.empty()) return 0;
    double sum = 0;
    for (size_t i = 0; i < list.size(); i++) sum += list[i];
    return sum / list.size();
}

std::vector<std::string> Grade::toCSV() const {
    std::vector<std::string> row;
    row.push_back(m_id);
    row.push_back(m_studentId);
    row.push_back(m_examId);
    row.push_back(m_scores);
    row.push_back(m_isMakeup ? "true" : "false");
    row.push_back(m_submittedBy);
    row.push_back(m_submittedAt);
    return row;
}

Grade Grade::fromCSV(const std::vector<std::string>& fields) {
    return Grade(fields);
}
