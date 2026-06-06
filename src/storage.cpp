// ==================== 数据持久化实现 ====================
// 本文件是 storage 命名空间中所有函数的实现代码。
// 物理上，数据存储在项目 data/ 目录下的 CSV 文件中，每个数据表对应一个 .csv 文件。
//
// 【fstream 读写原理简介】
// C++ 用 <fstream> 标准库提供的三个类来操作文件：
//   ifstream   = "input file stream" —— 从文件中读入数据（读模式）
//   ofstream   = "output file stream" —— 向文件中写出数据（写模式）
//   fstream    = 同时支持读和写
// 核心用法：构造对象→打开文件→读写→关闭（析构函数自动关闭）
//
// 【CSV 格式简介】
// CSV 全称 Comma-Separated Values（逗号分隔值），是一种纯文本表格格式。
// 规则很简单：每行是一条记录，不同字段用逗号隔开，第一行通常是列名（表头）。
// 例如 users.csv 的一行：
//   1,admin,123456,admin,13800000000,2024-01-01
// 这种格式可以像普通 txt 一样编辑，也可以用 Excel 打开。

#include "storage.h"
#include "utils.h"
#include <fstream>   // 文件流：提供 ifstream/ofstream 进行文件读写
#include <sstream>   // 字符串流：在内存中像读写文件一样操作字符串（本项目未直接使用但保留）

namespace storage {

    // ----- 拆分 CSV 行 -----
    // 调用 utils::split 按逗号拆分一行字符串。
    // 返回 vector<string>，即分割后的各个字段。
    // 例如输入 "101,张三,男" → 返回 {"101", "张三", "男"}
    std::vector<std::string> splitCSV(const std::string& line) {
        std::vector<std::string> result;
        std::string current;
        bool inQuotes = false;
        for (size_t i = 0; i < line.size(); i++) {
            char c = line[i];
            if (inQuotes) {
                if (c == '"') {
                    inQuotes = false;
                } else {
                    current += c;
                }
            } else {
                if (c == '"') {
                    inQuotes = true;
                } else if (c == ',') {
                    result.push_back(current);
                    current.clear();
                } else {
                    current += c;
                }
            }
        }
        result.push_back(current);
        return result;
    }

    // ----- 转义字段中的特殊字符 -----
    // CSV 以逗号为分隔符，如果某个字段的值本身就包含逗号，
    // 直接写入会破坏表格结构，让人误以为该逗号是分隔符。
    // 解决方法：用双引号把整个字段包裹起来。
    // string::npos 是 string 类定义的一个常量，表示"未找到"，值通常为最大的 size_t。
    // 例如输入 "张,三" → 返回 "\"张,三\""（写入文件后变成 "张,三"）
    std::string escapeField(const std::string& field) {
        if (field.find(',') != std::string::npos)  // 如果字段中含有逗号
            return "\"" + field + "\"";             // 用双引号包裹，防止被误认为分隔符
        return field;                                // 没有逗号则直接返回原字段
    }

    // ----- 读取整个 CSV 文件 -----
    // 这是所有 getXxx() 函数的底层实现。
    // 用 ifstream 打开文件，逐行读取，解析后存入二维数组返回。
    std::vector<std::vector<std::string>> readCSV(const std::string& filename) {
        std::vector<std::vector<std::string>> data;  // 二维数组，外层是行，内层是字段
        std::string path = utils::DATA_DIR + filename; // 拼接出完整路径，如 "data/users.csv"
        std::ifstream f(path);  // 以读模式打开文件
        if (!f.is_open()) return data;  // 文件不存在或无法访问时，返回空数据

        std::string line;
        // 跳过第一行（表头/列名，例如 "id,name,password,..."）
        if (std::getline(f, line)) {
            // 读取成功，line 中存的是表头，直接丢弃
        }
        while (std::getline(f, line)) {  // 逐行读取，每次读取一行到 line
            if (line.empty()) continue;   // 忽略空行（避免产生空记录）
            data.push_back(splitCSV(line)); // 拆分该行，追加到数据数组中
        }
        // ifstream 对象 f 在离开作用域时自动调用 close() 关闭文件
        return data;
    }

    // ----- 覆盖写入 CSV 文件 -----
    // 这是所有 saveXxx() 函数的底层实现。
    // 用 ofstream 打开文件（默认模式会清空原有内容，相当于"覆盖写"），
    // 把二维数组中每一行的字段用逗号拼接写入。
    void writeCSV(const std::string& filename, const std::vector<std::vector<std::string>>& data) {
        utils::ensureDataDir();  // 确保 data/ 目录存在，不存在则创建
        std::ofstream f(utils::DATA_DIR + filename); // 以写模式打开文件（不追加则默认覆盖）
        if (!f.is_open()) return;  // 打不开就放弃
        for (size_t i = 0; i < data.size(); i++) {       // 遍历每一行
            for (size_t j = 0; j < data[i].size(); j++) { // 遍历行内的每个字段
                if (j > 0) f << ",";                      // 不是第一个字段时，先输出一个逗号分隔
                f << escapeField(data[i][j]);             // 写入字段值（自动处理逗号转义）
            }
            f << "\n";  // 一行结束，输出换行符
        }
    }

    // ----- 追加一行到 CSV 文件末尾 -----
    // 与 writeCSV 的区别：使用 std::ios::app 模式打开，不会清空已有内容。
    // 常用于需要实时保存的场景，如录入一条新成绩时只需追加而不必重写整个文件。
    void appendCSVRow(const std::string& filename, const std::vector<std::string>& row) {
        utils::ensureDataDir();
        // std::ios::app = append（追加）模式，写入的数据自动附加到文件末尾
        std::ofstream f(utils::DATA_DIR + filename, std::ios::app);
        if (!f.is_open()) return;
        for (size_t j = 0; j < row.size(); j++) {  // 与 writeCSV 同样的拼接逻辑
            if (j > 0) f << ",";
            f << escapeField(row[j]);
        }
        f << "\n";
    }

    // ===== 用户数据存取（users.csv）=====
    // 存储所有系统用户：管理员、教师、学生
    // 字段：id | name | password | role | phone | created_at

    std::vector<std::vector<std::string>> getUsers() {
        return readCSV("users.csv");  // 读取文件，跳过表头，返回纯数据行
    }

    void saveUsers(const std::vector<std::vector<std::string>>& data) {
        std::vector<std::vector<std::string>> withHeader;
        withHeader.push_back({"id","name","password","role","phone","created_at"}); // 先写入表头
        for (size_t i = 0; i < data.size(); i++) withHeader.push_back(data[i]);     // 再追加所有数据行
        writeCSV("users.csv", withHeader);
    }

    // ===== 学生数据存取（students.csv）=====
    // 字段：id(学号) | name(姓名) | birthday(出生日期) | class(班级编号) | gender(性别) | enrolled_at(入学时间)

    std::vector<std::vector<std::string>> getStudents() {
        return readCSV("students.csv");
    }

    void saveStudents(const std::vector<std::vector<std::string>>& data) {
        std::vector<std::vector<std::string>> withHeader;
        withHeader.push_back({"id","name","birthday","class","gender","enrolled_at"});
        for (size_t i = 0; i < data.size(); i++) withHeader.push_back(data[i]);
        writeCSV("students.csv", withHeader);
    }

    // ===== 班级数据存取（classes.csv）=====
    // 字段：id(班级编号) | name(班级名称) | grade(年级) | created_at(创建时间)

    std::vector<std::vector<std::string>> getClasses() {
        return readCSV("classes.csv");
    }

    void saveClasses(const std::vector<std::vector<std::string>>& data) {
        std::vector<std::vector<std::string>> withHeader;
        withHeader.push_back({"id","name","grade","created_at"});
        for (size_t i = 0; i < data.size(); i++) withHeader.push_back(data[i]);
        writeCSV("classes.csv", withHeader);
    }

    // ===== 科目数据存取（subjects.csv）=====
    // 字段：id(科目编号) | name(科目名称) | full_score(满分分值)

    std::vector<std::vector<std::string>> getSubjects() {
        return readCSV("subjects.csv");
    }

    void saveSubjects(const std::vector<std::vector<std::string>>& data) {
        std::vector<std::vector<std::string>> withHeader;
        withHeader.push_back({"id","name","full_score"});
        for (size_t i = 0; i < data.size(); i++) withHeader.push_back(data[i]);
        writeCSV("subjects.csv", withHeader);
    }

    // ===== 考试数据存取（exams.csv）=====
    // 字段：id(考试编号) | name(考试名称) | date(日期) | status(状态) | subjects(科目列表) | weight_config(权重配置)

    std::vector<std::vector<std::string>> getExams() {
        return readCSV("exams.csv");
    }

    void saveExams(const std::vector<std::vector<std::string>>& data) {
        std::vector<std::vector<std::string>> withHeader;
        withHeader.push_back({"id","name","date","status","subjects","weight_config"});
        for (size_t i = 0; i < data.size(); i++) withHeader.push_back(data[i]);
        writeCSV("exams.csv", withHeader);
    }

    // ===== 成绩数据存取（grades.csv）=====
    // 字段：id(记录编号) | student_id(学号) | exam_id(考试编号) | scores(各科分数) | is_makeup(是否补考) | submitted_by(录入人) | submitted_at(录入时间)
    // 成绩表是核心——通过 student_id 关联学生表，通过 exam_id 关联考试表

    std::vector<std::vector<std::string>> getGrades() {
        return readCSV("grades.csv");
    }

    void saveGrades(const std::vector<std::vector<std::string>>& data) {
        std::vector<std::vector<std::string>> withHeader;
        withHeader.push_back({"id","student_id","exam_id","scores","is_makeup","submitted_by","submitted_at"});
        for (size_t i = 0; i < data.size(); i++) withHeader.push_back(data[i]);
        writeCSV("grades.csv", withHeader);
    }

}
