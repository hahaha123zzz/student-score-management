# 学生成绩管理系统重构设计规格

## 概述

将现有项目从"C++17 + 第三方库(cpp-httplib, nlohmann/json) + 函数式风格"重构为"C++11 + 纯标准库+WinSock + OOP类继承 + fstream CSV存储"，使其代码复杂度匹配初学C++的大二学生水平，同时满足课程评分要求。

---

## 技术栈变更

| 层面 | 重构前 | 重构后 |
|------|--------|--------|
| C++标准 | C++17 | C++11 |
| HTTP服务 | cpp-httplib (第三方库) | WinSock自写 (系统库) |
| 数据存储 | JSON文件 (nlohmann/json) | CSV文件 (fstream) |
| 代码组织 | namespace + 函数 | namespace + 函数 + OOP类继承 |
| 排序 | std::sort | 手写冒泡排序/快速排序 |
| 随机数 | <random> (mt19937) | rand() + srand(time(0)) |
| 平台支持 | 跨平台 (#ifdef) | Windows only |
| 路由分发 | lambda + 正则 (httplib) | if-else字符串匹配 |
| 前端 | HTML/CSS/JS | 不变 |

---

## 目录结构

```
D:\c++成绩管理系统\
├── include/
│   ├── utils.h         工具函数 (哈希、日志、字符串处理、时间)
│   ├── storage.h       CSV文件读写 (fstream)
│   ├── models.h        类定义 (Person, Admin, Student, Exam, Grade)
│   ├── handlers.h      所有API业务处理函数
│   ├── stats.h         统计分析 (排名、分布、对比、趋势、预警)
│   ├── sort.h          排序算法 (冒泡排序、快速排序)
│   └── server.h        WinSock HTTP服务端
│
├── src/
│   ├── main.cpp        入口点: 启动服务器 + URL路由分发 (if-else)
│   ├── utils.cpp
│   ├── storage.cpp
│   ├── models.cpp
│   ├── handlers.cpp
│   ├── stats.cpp
│   ├── sort.cpp
│   └── server.cpp
│
├── web/                (前端, 不变)
│   ├── login.html
│   ├── admin.html
│   ├── student.html
│   └── css/style.css
│
├── data/               CSV数据文件 (运行时自动生成)
│   ├── users.csv
│   ├── students.csv
│   ├── classes.csv
│   ├── subjects.csv
│   ├── exams.csv
│   └── grades.csv
│
└── CMakeLists.txt
```

---

## 模块设计

### 1. server.cpp — WinSock HTTP服务

**职责**: 监听端口8080, 接收HTTP请求, 解析请求行和请求头, 将请求分发给路由, 返回响应。

**设计要点**:
- 使用 WinSock2 API (WSAStartup, socket, bind, listen, accept, recv, send)
- 解析HTTP请求行 (GET /api/xxx HTTP/1.1)
- 只读请求体 (POST请求的body)
- 构建HTTP响应 (状态行 + Content-Type: application/json + CORS头 + 空行 + body)
- 单线程、阻塞式处理 (一次处理一个请求)
- 约100-150行

**接口**:
```cpp
void startServer();
```

---

### 2. storage.cpp — CSV文件读写

**职责**: 读写 data/ 目录下的CSV文件。每类数据一个CSV文件, 第一行为列名, 后续行为数据。

**设计要点**:
- 使用 fstream (ifstream / ofstream)
- 每个函数负责一张表的增删改查
- 不使用任何第三方库, 手动解析逗号分隔
- 使用 getline() 配合字符级解析处理逗号分隔

**CSV文件格式示例**:

users.csv:
```
id,name,password,role,created_at
1,admin,6527dd1705a,admin,2025-01-01 00:00:00
2,2025001,张三,6527dd1705a,student,2025-01-01 00:00:00
```

students.csv:
```
id,name,gender,birthday,class,enrolled_at
1,张三,男,2005-03-15,高一(1)班,2025-01-01 00:00:00
```

exams.csv:
```
id,name,date,status,subjects
1,期中考试,2025-04-15,draft,语文|数学|英语
```
> 多值字段使用 `|` 竖线分隔

grades.csv:
```
id,student_id,exam_id,scores,is_makeup
1,1,1,85.5|92.0|78.0,false
```
> scores 字段各科分数用 `|` 分隔, 顺序与 exam 的 subjects 一一对应

**接口** (命名空间 storage):
```cpp
// 工具函数
std::vector<std::string> splitCSV(const std::string& line);
std::string joinCSV(const std::vector<std::string>& fields);
std::string escapeCSV(const std::string& field);

// 通用CRUD
std::vector<std::vector<std::string>> readAll(const std::string& filename);
void writeAll(const std::string& filename, const std::vector<std::vector<std::string>>& data);
void appendRow(const std::string& filename, const std::vector<std::string>& row);

// 用户
std::vector<std::vector<std::string>> getUsers();
void saveUsers(const std::vector<std::vector<std::string>>& users);

// 学生
std::vector<std::vector<std::string>> getStudents();
void saveStudents(const std::vector<std::vector<std::string>>& students);

// 班级
std::vector<std::vector<std::string>> getClasses();
void saveClasses(const std::vector<std::vector<std::string>>& classes);

// 科目
std::vector<std::vector<std::string>> getSubjects();
void saveSubjects(const std::vector<std::vector<std::string>>& subjects);

// 考试
std::vector<std::vector<std::string>> getExams();
void saveExams(const std::vector<std::vector<std::string>>& exams);

// 成绩
std::vector<std::vector<std::string>> getGrades();
void saveGrades(const std::vector<std::vector<std::string>>& grades);

// 日志
void appendLog(const std::vector<std::string>& logRow);
std::vector<std::vector<std::string>> getLogs();
```

---

### 3. models.cpp — 类定义

**职责**: 定义核心数据结构的类, 封装属性和方法, 使用继承体现OOP特性。

**设计要点**:
- 浅层继承, 不使用虚函数、纯虚基类
- 继承关系: Person -> Admin / Student
- Exam 和 Grade 为独立类
- 每个类提供 getter/setter 方法体现封装

**类设计**:

```cpp
class Person {
protected:
    int id;
    std::string name;
    std::string password;
    std::string role;
    std::string createdAt;
public:
    Person();
    Person(int id, const std::string& name, const std::string& password,
           const std::string& role, const std::string& createdAt);

    // getters / setters
    int getId() const;
    std::string getName() const;
    std::string getPassword() const;
    std::string getRole() const;
    void setName(const std::string& name);
    void setPassword(const std::string& password);

    // 认证
    bool verifyPassword(const std::string& input) const;

    // 序列化
    std::vector<std::string> toCSV() const;
    static Person fromCSV(const std::vector<std::string>& fields);
};

class Admin : public Person {
public:
    Admin();
    Admin(int id, const std::string& name, const std::string& password,
          const std::string& createdAt);
};

class Student : public Person {
private:
    std::string className;
    std::string birthday;
    std::string gender;
    std::string enrolledAt;
public:
    Student();
    Student(const std::vector<std::string>& fields);

    std::string getClassName() const;
    std::string getBirthday() const;
    std::string getGender() const;

    void setClassName(const std::string& cls);

    std::vector<std::string> toCSV() const;
    static Student fromCSV(const std::vector<std::string>& fields);
};

class Exam {
private:
    int id;
    std::string name;
    std::string date;
    std::string status;  // draft / published / locked
    std::vector<std::string> subjects;
public:
    Exam();
    Exam(const std::vector<std::string>& fields);

    // getters/setters
    int getId() const;
    std::string getName() const;
    std::string getDate() const;
    std::string getStatus() const;
    std::vector<std::string> getSubjects() const;

    void setName(const std::string& name);
    void setDate(const std::string& date);
    void setStatus(const std::string& status);
    void setSubjects(const std::vector<std::string>& subs);

    // 状态转换
    bool publish();
    bool retract();
    bool lock();

    std::vector<std::string> toCSV() const;
    static Exam fromCSV(const std::vector<std::string>& fields);
};

class Grade {
private:
    int id;
    int studentId;
    int examId;
    std::vector<double> scores;
    bool isMakeup;
public:
    Grade();
    Grade(const std::vector<std::string>& fields);

    int getId() const;
    int getStudentId() const;
    int getExamId() const;
    std::vector<double> getScores() const;
    bool getIsMakeup() const;

    void setScores(const std::vector<double>& scores);
    void setIsMakeup(bool makeup);

    double calcTotal() const;
    double calcAverage() const;

    std::vector<std::string> toCSV() const;
    static Grade fromCSV(const std::vector<std::string>& fields);
};
```

---

### 4. handlers.cpp — API业务处理

**职责**: 实现所有API端点的业务逻辑, 接收请求参数, 调用 storage 和 models, 返回JSON响应字符串。

**设计要点**:
- 每个功能一个独立函数, 职责单一
- 返回 std::string (JSON格式的响应体)
- 使用 utils::jsonResponse() 构建统一格式响应
- 不使用lambda, 不用中间件模式
- Admin验证通过一个简单的 `isAdmin(const std::string& token)` 函数

**已确定的API列表** (约30个):

认证模块:
- `POST /api/login` — 用户名+密码验证, 返回token
- `POST /api/change-password` — 修改密码
- `GET /api/auth/check` — 检查token有效性

用户管理 (Admin):
- `GET /api/users` — 用户列表
- `POST /api/users` — 添加用户
- `PUT /api/users` — 修改用户
- `DELETE /api/users` — 删除用户

学生管理:
- `GET /api/students` — 学生列表 (分页+搜索+班级过滤)
- `POST /api/students` — 添加学生
- `PUT /api/students` — 修改学生
- `DELETE /api/students` — 删除学生

班级管理:
- `GET /api/classes` — 班级列表
- `POST /api/classes` — 添加班级
- `PUT /api/classes` — 修改班级
- `DELETE /api/classes` — 删除班级

考试管理:
- `GET /api/exams` — 考试列表
- `POST /api/exams` — 添加考试
- `PUT /api/exams` — 修改考试
- `DELETE /api/exams` — 删除考试
- `PUT /api/exams/{id}/publish` — 发布考试
- `PUT /api/exams/{id}/retract` — 撤回考试

成绩管理:
- `POST /api/grades` — 录入/更新成绩
- `POST /api/grades/import` — CSV批量导入
- `GET /api/grades/export` — CSV导出
- `PUT /api/grades/{id}/lock` — 锁定成绩
- `GET /api/grades` — 查询成绩列表
- `GET /api/grades/student/{id}` — 学生历史成绩
- `POST /api/grades/makeup` — 补考成绩

统计分析:
- `GET /api/stats/rank` — 成绩排名
- `GET /api/stats/class-compare` — 班级对比
- `GET /api/stats/distribution` — 分数分布
- `GET /api/stats/trend/{id}` — 成绩趋势
- `GET /api/stats/warnings` — 不及格预警
- `GET /api/stats/enrollment` — 各班人数统计

日志:
- `GET /api/logs` — 操作日志

**模块内文件拆分**:
- `handlers.cpp` 统一管理所有handler函数
- `stats.cpp` 从handlers中抽离统计分析部分 (算法密集)
- 不依赖任何第三方库

---

### 5. utils.cpp — 工具函数

**职责**: 跨模块共享的基础工具函数。

**保留的功能**:
- DJB2密码哈希 (hashPassword, verifyPassword)
- 操作日志 (appendLog, getLogs)
- 时间格式化 (getCurrentTime)
- 字符串处理 (toLower, trim)
- 数学工具 (round2, toPercent, gradeToLetter, gradeToGPA)
- JSON响应构建 (jsonResponse, successResponse, errorResponse)
- API参数解析 (getQueryParam, parseRequestBody)

**去除的功能**:
- 跨平台文件操作 (只保留Windows版本)
- JSON文件读写 (改为storage.cpp的CSV读写)
- `<random>` 随机数 (改为rand/srand)

**新增/修改的功能**:
- Token生成: 使用 `rand()` 替代 `std::mt19937`, 生成16位十六进制随机字符串
- Token存储: 使用内存中的 `std::map<std::string, int>` (token -> userId), 重启后失效
- 响应格式: 手动拼接JSON字符串, 替代 nlohmann/json
  ```cpp
  // 统一响应: {"success":true,"message":"ok","data":{...}}
  std::string jsonResponse(bool success, const std::string& message, const std::string& dataJson);
  ```
- 日志存储: 写入 `data/logs.txt`, 每行一个日志条目, `|` 分隔字段

**JSON格式**: 由于不引入nlohmann/json, 手动构建JSON字符串:
```cpp
// 简单JSON构建
std::string jsonResponse(bool success, const std::string& message, const std::string& dataJson);
// dataJson 为调用者预先构建的JSON片段
```

---

### 6. sort.cpp — 排序算法

**职责**: 提供手写的排序算法, 满足课程对算法的要求。

**设计要点**:
- 实现冒泡排序 (用于教学演示)
- 实现快速排序 (用于实际排序, 性能更好)
- 提供对 `std::vector<double>` 排序的函数
- 提供带索引排序的函数 (排序时保留原始索引用于排名)

**接口**:
```cpp
namespace sort {
    // 冒泡排序 (升序)
    void bubbleSort(double arr[], int size);
    void bubbleSort(std::vector<double>& arr);

    // 快速排序 (升序)
    void quickSort(double arr[], int low, int high);
    void quickSort(std::vector<double>& arr);

    // 带索引排序 (用于排名, 排完后 indices 中按分数从高到低排列)
    void quickSortByScore(std::vector<double>& scores, std::vector<int>& indices);
}
```

---

### 7. stats.cpp — 统计分析

**职责**: 实现成绩排名、班级对比、分数分布、成绩趋势、不及格预警等统计分析功能。

**设计要点**:
- 从 CSV 数据中读取成绩和相关信息
- 使用 handers 模块计算加权总分
- 使用 sort 模块进行排序 (而非 std::sort)
- 不使用结构化绑定 (`auto& [key, value]`), 改用传统迭代
- 不使用 lambda 表达式, 改用独立比较函数

---

### 8. main.cpp — 入口点

**职责**: 程序初始化, 路由分发, 启动服务器。

**设计要点**:
- 如果 data/ 目录不存在, 生成种子数据
- 启动 WinSock 服务器
- 路由分发使用 if-else 字符串匹配:

```cpp
// 路由分发 (约30个 if-else 分支)
std::string route(const std::string& method, const std::string& path,
                  const std::string& body, const std::string& token) {
    if (method == "POST" && path == "/api/login") {
        return handlers::login(body);
    }
    else if (method == "POST" && path == "/api/change-password") {
        return handlers::changePassword(token, body);
    }
    else if (method == "GET" && path == "/api/students") {
        return handlers::getStudents(body);
    }
    // ... 约30个路由
    else if (path.find("/web/") == 0 || path == "/") {
        return serveStaticFile(path);  // 静态文件服务
    }
    return utils::errorResponse(404, "Not Found");
}
```

- 约80-100行, 结构清晰, 无lambda
- 请求体参数通过URL query string传递 (GET) 或 POST body传递

---

## 前端变更

**不变。** 前端 login.html, admin.html, student.html, style.css 保持不变。

但需注意:
- 前端 fetch() 发请求到相同的API路径 (/api/...)
- 后端返回的JSON格式保持与之前一致: `{"success": true/false, "message": "...", "data": {...}}`
- 确保 CORS 响应头正确设置

---

## 数据流

```
浏览器 (HTML/JS)
    |
    | HTTP请求 (GET/POST)
    v
server.cpp (WinSock 接收请求)
    |
    | method + path + body
    v
main.cpp (路由分发, if-else)
    |
    | 分发到对应handler
    v
handlers.cpp (业务逻辑)
    |
    | 调用 storage 读CSV, 调用 stats 计算, 调用 sort 排序
    v
    | 返回 JSON 字符串
    v
main.cpp -> server.cpp -> 浏览器
```

---

## 构建配置

```cmake
cmake_minimum_required(VERSION 3.10)
project(EduGrade)

set(CMAKE_CXX_STANDARD 11)      # 降级到 C++11
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 不再需要 include/lib/ (去掉了第三方库)
include_directories(include)

file(GLOB SOURCES src/*.cpp)

add_executable(edugrade ${SOURCES})

target_link_libraries(edugrade ws2_32)  # WinSock
```

---

## 小组分工建议 (4-5人)

| 组员 | 负责模块 | 难度 |
|------|---------|------|
| 组员A | server.cpp (WinSock) + CMakeLists.txt | 中 |
| 组员B | storage.cpp (CSV读写) + utils.cpp | 中 |
| 组员C | models.cpp (类定义) + handlers.cpp (认证+用户+学生) | 低 |
| 组员D | handlers.cpp (考试+成绩) + sort.cpp | 低 |
| 组员E | stats.cpp + handlers.cpp (统计API) | 中 |

---

## 与课程要求的对应检查

| 课程要求 | 本项目满足方式 | 状态 |
|----------|---------------|:--:|
| 面向对象程序设计 (OOP) | Person -> Admin/Student 继承体系 | ✓ |
| 排序算法 | 手写冒泡排序 + 快速排序 | ✓ |
| 文件存储 (fstream) | CSV 文件存储 | ✓ |
| 控制台菜单 / Web界面 | HTML/CSS/JS 前端 | ✓ |
| 管理员/学生角色 | Person 子类 + token认证 | ✓ |
| 成绩排名 | stats.cpp | ✓ |
| 安全性 (密码验证) | DJB2 哈希 + token 认证 | ✓ |
| 前端 + C++ 后端 | WinSock HTTP + HTML | ✓ |
| 通信方式 (建议轻量HTTP) | WinSock 自写HTTP服务 | ✓ |
