#pragma once

#include <string>
#include <vector>

class Person {
protected:
    std::string m_id;
    std::string m_name;
    std::string m_password;
    std::string m_role;
    std::string m_createdAt;

public:
    Person();
    Person(const std::string& id, const std::string& name, const std::string& password,
           const std::string& role, const std::string& createdAt);

    std::string getId() const;
    std::string getName() const;
    std::string getPassword() const;
    std::string getRole() const;
    std::string getCreatedAt() const;

    void setName(const std::string& name);
    void setPassword(const std::string& password);
    void setRole(const std::string& role);

    bool verifyPassword(const std::string& input) const;

    std::vector<std::string> toCSV() const;
    static Person fromCSV(const std::vector<std::string>& fields);
};

class Admin : public Person {
public:
    Admin();
    Admin(const std::string& id, const std::string& name, const std::string& password,
          const std::string& createdAt);
};

class Student : public Person {
private:
    std::string m_className;
    std::string m_birthday;
    std::string m_gender;
    std::string m_enrolledAt;

public:
    Student();
    Student(const std::vector<std::string>& fields);

    std::string getClassName() const;
    std::string getBirthday() const;
    std::string getGender() const;

    void setClassName(const std::string& cls);
    void setBirthday(const std::string& birthday);
    void setGender(const std::string& gender);

    std::vector<std::string> toCSV() const;
    static Student fromCSV(const std::vector<std::string>& fields);
};

class Cls {
private:
    std::string m_id;
    std::string m_name;
    std::string m_grade;
    std::string m_createdAt;

public:
    Cls();
    Cls(const std::string& id, const std::string& name,
        const std::string& grade, const std::string& createdAt);

    std::string getId() const;
    std::string getName() const;
    std::string getGrade() const;

    void setName(const std::string& name);
    void setGrade(const std::string& grade);

    std::vector<std::string> toCSV() const;
    static Cls fromCSV(const std::vector<std::string>& fields);
};

class Subject {
private:
    std::string m_id;
    std::string m_name;
    int m_fullScore;

public:
    Subject();
    Subject(const std::string& id, const std::string& name, int fullScore);

    std::string getId() const;
    std::string getName() const;
    int getFullScore() const;

    std::vector<std::string> toCSV() const;
    static Subject fromCSV(const std::vector<std::string>& fields);
};

class Exam {
private:
    std::string m_id;
    std::string m_name;
    std::string m_date;
    std::string m_status;
    std::string m_subjects;
    std::string m_weightConfig;

public:
    Exam();
    Exam(const std::string& id, const std::string& name, const std::string& date,
         const std::string& status, const std::string& subjects, const std::string& weightConfig);

    std::string getId() const;
    std::string getName() const;
    std::string getDate() const;
    std::string getStatus() const;
    std::string getSubjects() const;
    std::string getWeightConfig() const;

    std::vector<std::string> getSubjectList() const;

    void setName(const std::string& name);
    void setDate(const std::string& date);
    void setStatus(const std::string& status);
    void setSubjects(const std::string& subjects);
    void setWeightConfig(const std::string& wc);

    bool publish();
    bool retract();
    bool lock();

    double getSubjectWeight(const std::string& subject) const;
    double getSubjectFull(const std::string& subject) const;

    std::vector<std::string> toCSV() const;
    static Exam fromCSV(const std::vector<std::string>& fields);
};

class Grade {
private:
    std::string m_id;
    std::string m_studentId;
    std::string m_examId;
    std::string m_scores;
    bool m_isMakeup;
    std::string m_submittedBy;
    std::string m_submittedAt;

public:
    Grade();
    Grade(const std::vector<std::string>& fields);

    std::string getRowId() const;
    std::string getStudentId() const;
    std::string getExamId() const;
    std::string getScores() const;
    bool getIsMakeup() const;

    double getScoreByIndex(int index) const;
    std::vector<double> getScoreList() const;

    void setScores(const std::string& scores);
    void setIsMakeup(bool makeup);

    double calcTotal(const std::string& weightConfig) const;
    double calcAverage() const;

    std::vector<std::string> toCSV() const;
    static Grade fromCSV(const std::vector<std::string>& fields);
};
