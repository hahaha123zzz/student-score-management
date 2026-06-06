# 学生成绩管理系统

> 一套用 C++11 纯标准库构建的 Web 成绩管理系统，适合大一C++课程设计。

---

## 目录

- [项目简介](#项目简介)
- [技术栈](#技术栈)
- [快速开始](#快速开始)
- [项目结构](#项目结构)
- [核心概念](#核心概念)
- [代码架构](#代码架构)
- [API 接口一览](#api-接口一览)
- [默认账号](#默认账号)
- [模块详解](#模块详解)
- [常见问题](#常见问题)

---

## 项目简介

这是一个**基于 C++11 编写的 HTTP 服务器**，提供完整的学生成绩管理功能。你可以通过浏览器访问网页界面，进行以下操作：

- 学生档案管理（增删改查）
- 班级管理
- 科目管理
- 考试管理（创建、发布、撤回、锁定）
- 成绩录入（手动录入 + CSV 批量导入导出）
- 补考成绩管理
- 统计分析（排名、班级对比、分数段分布）
- 不及格预警
- 操作日志

**第一次运行时会自动生成 50 名学生的测试数据**，可以直接用来演示和测试。

---

## 技术栈

| 层     | 技术                        | 说明                      |
| ------ | --------------------------- | ------------------------- |
| 语言   | C++11                       | 基础编程语言              |
| HTTP   | 自写 Socket（WinSock/POSIX）| 用 Socket 构建 HTTP 服务  |
| 存储   | CSV 文件（fstream）          | 纯文本表格格式，无需数据库 |
| 前端   | 原生 HTML + CSS + JS        | 无需前端框架              |
| 构建   | CMake / g++ / MSVC          | 跨平台（Windows/macOS/Linux）|

**零第三方依赖**，所有功能用 C++ 标准库 + 系统自带的 WinSock2 实现。

---

## 快速开始

### 方式一：直接运行（Windows）

直接双击 `edugrade.exe`，然后在浏览器打开 **http://localhost:8080**。

### 方式二：用 CMake 编译

```bash
cmake -B build
cmake --build build
# Windows
build\edugrade.exe
# macOS / Linux
./build/edugrade
```

### 方式三：手动编译

**Windows（MSVC）：** 打开 Developer Command Prompt

```cmd
cl /EHsc /utf-8 src\*.cpp /I include /Fe:edugrade.exe ws2_32.lib
```

**macOS / Linux（g++）：**

```bash
g++ -std=c++11 -o edugrade src/*.cpp -I include -lpthread
```

### 方式三：手动编译（MSVC）

打开 Visual Studio Developer Command Prompt，然后：

```cmd
cl /EHsc /utf-8 src\*.cpp /I include /Fe:edugrade.exe ws2_32.lib
```

运行后在浏览器访问 **http://localhost:8080** 进入登录页。

---

## 项目结构

```
c++成绩管理系统/
├── include/                      # 头文件（函数声明）
│   ├── utils.h                   # 工具函数（哈希、Token、JSON构建、字符串）
│   ├── storage.h                 # CSV 文件读写（fstream）
│   ├── models.h                  # OOP 类定义（Person/Admin/Student/Exam/Grade）
│   ├── handlers.h                # 所有 API 业务处理函数
│   ├── stats.h                   # 统计分析（排名、对比、分布）
│   ├── sort.h                    # 手写排序算法（冒泡、快速排序）
│   └── server.h                  # WinSock HTTP 服务端
│
├── src/                          # 源文件（函数实现）
│   ├── main.cpp                  # 程序入口 + 种子数据 + 路由分发
│   ├── utils.cpp                 # 工具函数实现
│   ├── storage.cpp               # CSV 读写实现
│   ├── models.cpp                # 类的实现 + CSV 序列化
│   ├── handlers.cpp              # 所有 API 处理逻辑
│   ├── stats.cpp                 # 统计分析实现
│   ├── sort.cpp                  # 排序算法实现
│   └── server.cpp                # WinSock HTTP 服务实现
│
├── web/                          # 前端页面
│   ├── login.html                # 登录页
│   ├── admin.html                # 管理员后台
│   ├── student.html              # 学生中心
│   └── css/style.css             # 样式表
│
├── data/                         # 数据文件（运行时自动生成）
│   ├── users.csv                 # 用户账号
│   ├── students.csv              # 学生档案
│   ├── classes.csv               # 班级信息
│   ├── subjects.csv              # 科目设置
│   ├── exams.csv                 # 考试记录
│   ├── grades.csv                # 成绩数据
│   └── logs.txt                  # 操作日志
│
├── CMakeLists.txt                # CMake 构建配置
└── README.md                     # 你正在看的这个文件
```

---

## 核心概念

### 1. HTTP 服务器是什么？

这个程序**本身就是一个网站服务器**。当你运行它后，它会在你电脑的 8080 端口上监听，等待浏览器发来的请求。所以你用浏览器访问 `http://localhost:8080`，其实是和这个 C++ 程序在通信。

### 2. 什么是 API？

API（Application Programming Interface）是一套"接口约定"。比如：
- 前端说："我要查学生列表，第 1 页，每页 20 条"
- 后端就返回对应的数据

在这个项目中，API 就是那些 `/api/login`、`/api/students` 这样的 URL。

### 3. Token 是什么？

Token 是用户登录后服务器给的一张"临时通行证"。把它想象成：
1. 你去酒店前台登记（登录），前台给你一张房卡（Token）
2. 之后每次进房间（访问功能），刷一下房卡就行了，不用重新登记
3. 房卡过期了（重启服务器），就要重新登记

### 4. CSV 是什么？

CSV (Comma-Separated Values) 是一种纯文本表格格式。每行一条记录，用逗号分隔字段：

```csv
学号,姓名,班级,性别
S2024001,张三,高一1班,男
S2024002,李四,高一1班,女
```

可以直接用记事本或 Excel 打开查看和编辑。所有数据（学生、成绩、用户等）都以 CSV 文件格式存储在 `data/` 目录下。

### 5. 为什么用 CSV 文件而不是数据库？

对于课程设计级别的项目：
- CSV 文件**简单直观**，可以直接用记事本打开查看
- 不需要安装和配置数据库
- 不需要学习 SQL 语言
- 数据量小（几百个学生）时性能完全够用
- 与课程要求的 fstream 文件操作直接对应

### 6. 哈希是什么？为什么要哈希存密码？

哈希就像"数字指纹"：`hashPassword("123456")` 永远等于同一个值，但你无法从哈希值反推出原始密码是 `123456`。

这样即使有人拿到了 users.csv 文件，也看不到真实密码。

---

## 代码架构

整个项目分为 **7 个模块**，每个模块一个头文件 + 一个源文件：

```
main.cpp  ← 入口，种子数据生成，路由分发
  ├── utils.h / utils.cpp         ← 基础工具（其他模块都依赖它）
  │     ├── storage.h / storage.cpp ← CSV 文件读写
  │     ├── models.h / models.cpp   ← OOP 类定义
  │     ├── handlers.h / handlers.cpp ← API 业务逻辑
  │     ├── stats.h / stats.cpp     ← 统计分析（调用 sort）
  │     ├── sort.h / sort.cpp       ← 手写排序算法
  │     └── server.h / server.cpp   ← WinSock HTTP 服务
```

### 模块依赖关系

```
main.cpp ──→ handlers ──→ storage ──→ utils
         ──→ stats    ──→ storage, sort ──→ utils
         ──→ server   ──→ (WinSock2)
```

### OOP 类继承体系

```
        Person（基类）
        ├── id, name, password, role
        ├── verifyPassword()
        └── toCSV() / fromCSV()
              │
    ┌─────────┴─────────┐
    │                   │
  Admin              Student
  (管理员)            ├── className, birthday, gender
                     └── getClassName() / getClass() ...

  Exam                 Cls（班级）
  ├── id, name         ├── id, name, grade
  ├── date, status
  ├── subjects[]       Grade（成绩）
  ├── publish()        ├── studentId, examId
  └── lock()           ├── scores[], isMakeup
                       └── calcTotal() / calcAverage()
```

### 程序的执行流程

```
main() 被调用
  │
  ├─ ① seedFullData()        → 检查并生成测试数据（CSV 文件）
  │
  ├─ ② server::setRouteHandler(route)  → 注册路由分发函数
  │
  └─ ③ server::start(webDir) → 启动 WinSock HTTP 服务
        │
        └─ 用户访问 http://localhost:8080
             └─ 服务器返回 web/login.html
             └─ 用户填写表单，点登录
             └─ 浏览器 POST /api/login
             └─ 服务器调用 handlers::login()
             └─ 返回 Token 给浏览器
             └─ 浏览器存下 Token，后续请求都带着它
```

### 一个请求的完整处理过程

以"查看学生列表"为例：

```
浏览器 GET /api/students?page=1&size=20
    │
    ▼
[server.cpp 收到 HTTP 请求]
    │
    ▼
[解析请求行] → method=GET, path=/api/students, queryString=page=1&size=20
    │
    ▼
[main.cpp route() 函数分发]
    if (path == "/api/students")
        → handlers::getStudents(queryString)
    │
    ▼
[handlers::getStudents]
    ├── storage::getStudents()           ← 读 students.csv
    ├── 遍历筛选（按姓名/班级）            ← 业务逻辑
    ├── 计算分页                          ← 业务逻辑
    └── return JSON 字符串                ← 返回
    │
    ▼
[server.cpp 构建 HTTP 响应 + CORS 头，发送回浏览器]
    │
    ▼
[浏览器收到 JSON 数据，渲染成表格]
```

---

## API 接口一览

所有接口返回统一格式：`{ success: bool, message: string, data: any }`

### 认证模块

| 方法   | URL                    | 说明           | 权限   |
| ------ | ---------------------- | -------------- | ------ |
| POST   | `/api/login`           | 用户登录       | 所有人 |
| POST   | `/api/change-password` | 修改密码       | 已登录 |
| GET    | `/api/auth/check`      | 检查登录状态   | 已登录 |

### 用户管理

| 方法   | URL               | 说明         | 权限   |
| ------ | ----------------- | ------------ | ------ |
| GET    | `/api/users`      | 用户列表     | 所有人 |
| POST   | `/api/users`      | 添加用户     | 所有人 |
| PUT    | `/api/users/{id}` | 修改用户     | 所有人 |
| DELETE | `/api/users/{id}` | 删除用户     | 所有人 |

### 学生管理

| 方法   | URL                  | 说明             | 权限   |
| ------ | -------------------- | ---------------- | ------ |
| GET    | `/api/students`      | 学生列表（分页） | 所有人 |
| POST   | `/api/students`      | 添加学生         | 所有人 |
| PUT    | `/api/students/{id}` | 修改学生         | 所有人 |
| DELETE | `/api/students/{id}` | 删除学生         | 所有人 |

### 班级管理

| 方法   | URL                 | 说明     | 权限   |
| ------ | ------------------- | -------- | ------ |
| GET    | `/api/classes`      | 班级列表 | 所有人 |
| POST   | `/api/classes`      | 添加班级 | 所有人 |
| PUT    | `/api/classes/{id}` | 修改班级 | 所有人 |
| DELETE | `/api/classes/{id}` | 删除班级 | 所有人 |

### 科目管理

| 方法 | URL              | 说明     | 权限   |
| ---- | ---------------- | -------- | ------ |
| GET  | `/api/subjects`  | 科目列表 | 所有人 |
| POST | `/api/subjects`  | 添加科目 | 所有人 |

### 考试管理

| 方法   | URL                       | 说明       | 权限   |
| ------ | ------------------------- | ---------- | ------ |
| GET    | `/api/exams`              | 考试列表   | 所有人 |
| POST   | `/api/exams`              | 创建考试   | 所有人 |
| PUT    | `/api/exams/{id}`         | 修改考试   | 所有人 |
| DELETE | `/api/exams/{id}`         | 删除考试   | 所有人 |
| PUT    | `/api/exams/{id}/publish` | 发布成绩   | 所有人 |
| PUT    | `/api/exams/{id}/retract` | 撤回成绩   | 所有人 |

### 成绩管理

| 方法   | URL                         | 说明             | 权限   |
| ------ | --------------------------- | ---------------- | ------ |
| POST   | `/api/grades`               | 录入/更新成绩    | 所有人 |
| POST   | `/api/grades/import`        | CSV 批量导入成绩 | 所有人 |
| GET    | `/api/grades/export`        | CSV 导出成绩     | 所有人 |
| PUT    | `/api/grades/{id}/lock`     | 锁定成绩         | 所有人 |
| GET    | `/api/grades`               | 查询成绩         | 所有人 |
| GET    | `/api/grades/student/{id}`  | 学生历史成绩     | 所有人 |
| POST   | `/api/grades/makeup`        | 补考成绩录入     | 所有人 |

### 统计分析

| 方法 | URL                        | 说明       | 权限   |
| ---- | -------------------------- | ---------- | ------ |
| GET  | `/api/stats/rank`          | 排名       | 所有人 |
| GET  | `/api/stats/class-compare` | 班级对比   | 所有人 |
| GET  | `/api/stats/distribution`  | 分数段分布 | 所有人 |
| GET  | `/api/stats/trend/{id}`    | 成绩趋势   | 所有人 |
| GET  | `/api/stats/warnings`      | 不及格预警 | 所有人 |
| GET  | `/api/stats/enrollment`    | 入学统计   | 所有人 |

### 其他

| 方法 | URL        | 说明     | 权限   |
| ---- | ---------- | -------- | ------ |
| GET  | `/api/logs` | 操作日志 | 所有人 |

---

## 默认账号

首次运行时会自动生成以下账号，**所有密码都是 `123456`**：

| 用户名    | 角色     | 说明         |
| --------- | -------- | ------------ |
| `admin`   | 管理员   | 超级管理员   |
| `T001`    | 管理员   | 张明老师     |
| `T002`    | 管理员   | 李华老师     |
| `T003`    | 管理员   | 王芳老师     |
| `S2024001` | 学生    | 张三         |
| `S2024002` | 学生    | 李四         |
| ...       | 学生    | 共50个学生   |

---

## 模块详解

### utils（工具模块）— `utils.h` / `utils.cpp`

**定位：** 最底层的工具箱，所有模块都依赖它。

**主要功能：**
- `hashPassword` — DJB2 哈希算法，用于密码加密存储
- `generateToken` — 生成 16 位随机 Token（使用 rand()）
- `storeToken / isTokenValid / getUserIdByToken` — Token 内存存储与验证
- `logAction` — 操作日志记录到 logs.txt
- `getCurrentTime` — 获取格式化时间字符串
- `split / join / toLower / trim` — 字符串处理工具
- `round2 / gradeToLetter / gradeToGPA` — 数学工具
- `jsonResponse / errorResponse` — 手动拼接 JSON 响应字符串
- `getQueryParam` — URL 查询参数解析

### storage（数据存储模块）— `storage.h` / `storage.cpp`

**定位：** 使用 fstream 读写 CSV 文件，每类数据一个 CSV 文件。

**主要功能：**
- `readCSV / writeCSV` — 底层 CSV 文件读写
- `splitCSV` — CSV 行按逗号解析
- `getUsers / saveUsers` — 用户数据
- `getStudents / saveStudents` — 学生数据
- `getClasses / saveClasses` — 班级数据
- `getSubjects / saveSubjects` — 科目数据
- `getExams / saveExams` — 考试数据
- `getGrades / saveGrades` — 成绩数据

**CSV 多值字段设计：** 考试科目和成绩分数用 `|` 分隔：

```
exams.csv  subjects 列:  语文|数学|英语|物理|化学
grades.csv scores 列:    120|135|110|85|92
```

### models（数据模型模块）— `models.h` / `models.cpp`

**定位：** 定义 OOP 类，体现封装和继承。

**类体系：**
- `Person`（基类）→ `Admin`（管理员）、`Student`（学生）— 继承关系
- `Cls`（班级）— 独立类
- `Subject`（科目）— 独立类
- `Exam`（考试）— 含状态机 draft/published/locked
- `Grade`（成绩）— 含加权总分计算

每个类提供 `toCSV()` / `fromCSV()` 方法完成与 CSV 文件的双向转换。

### handlers（业务逻辑模块）— `handlers.h` / `handlers.cpp`

**定位：** 实现所有 API 的业务逻辑，是系统核心。

**主要功能：**
- 认证：`login`、`changePassword`、`checkAuth`、`isAdmin`
- 用户 CRUD：`getUsers`、`addUser`、`updateUser`、`deleteUser`
- 学生 CRUD：`getStudents`（分页+搜索）、`addStudent`、`updateStudent`、`deleteStudent`
- 班级 CRUD：`getClasses`、`addClass`、`updateClass`、`deleteClass`
- 科目管理：`getSubjects`、`addSubject`
- 考试管理：`getExams`、`addExam`、`updateExam`、`deleteExam`、`publishExam`、`retractExam`
- 成绩管理：`setGrades`、`importCSV`、`exportCSV`、`lockGrades`、`getGrades`、`markMakeup`

### sort（排序算法模块）— `sort.h` / `sort.cpp`

**定位：** 手写排序算法，满足课程对算法的要求。

**实现的算法：**
- `bubbleSort` — 冒泡排序（O(n²)，用于教学演示）
- `quickSort` — 快速排序（O(n log n)，用于实际排序）
- `quickSortWithIndex` — 带索引的快速排序（用于排名，分数排序同时记录原始位置）

### stats（统计分析模块）— `stats.h` / `stats.cpp`

**定位：** 对成绩数据进行深度分析。

**主要功能：**
- `getRank` — 排名（总分/单科，全年级/班级内），使用手写快速排序
- `getClassCompare` — 班级对比（平均分、优秀率、良好率、及格率、最高/最低/中位数）
- `getDistribution` — 分数段分布（<60, 60-69, 70-79, 80-89, 90-100）
- `getTrend` — 学生成绩趋势（多场考试的变化）
- `getWarnings` — 不及格预警
- `getEnrollmentStats` — 各班级人数统计

### server（HTTP 服务模块）— `server.h` / `server.cpp`

**定位：** 基于 WinSock2 的手写 HTTP 服务器。

**实现要点：**
- `WSAStartup → socket → bind → listen → accept` 标准 Socket 流程
- 解析 HTTP 请求行、请求头（Authorization 提取 Token）、请求体
- 自动区分静态文件请求（返回 HTML/CSS/JS）和 API 请求（调用 handler）
- 添加 CORS 跨域响应头
- 单线程阻塞模式，简单易懂

### main.cpp（程序入口）

**定位：** 程序的"大脑"，负责串联一切。

**做的事情：**
1. `seedFullData()` — 初始化测试数据（生成 CSV 文件）
2. `route()` — 路由分发函数，用 if-else 匹配 URL
3. `server::setRouteHandler(route)` — 注册路由
4. `server::start(webDir)` — 启动 WinSock 服务器监听 8080 端口

---

## 常见问题

### Q: 怎么改端口号？

在 `src/server.cpp` 中找到 `htons(8080)`，把 `8080` 改成你想要的端口号，重新编译。

### Q: 数据保存在哪里？

所有数据都保存在 `data/` 目录下的 CSV 文件中。可以直接用记事本或 Excel 打开查看。

### Q: 怎么重置所有数据？

删除 `data/` 目录，重新运行程序，它会自动生成全新的测试数据。

### Q: 为什么密码看不到？

密码存储的是哈希值（数字指纹），不是明文。这是安全设计——即使数据文件泄露，也无法知道真实密码。

### Q: 前端页面怎么自己修改？

前端页面在 `web/` 目录下，是纯 HTML + CSS + JavaScript，可以直接编辑。修改后刷新浏览器即可看到效果，不需要重新编译。

### Q: 怎么添加新功能？

以添加一个"导出排名为 Excel"的功能为例：
1. 在 `src/handlers.cpp` 中写一个新函数（如 `exportRankExcel`）
2. 在 `include/handlers.h` 中声明它
3. 在 `src/main.cpp` 的 `route()` 函数中添加一个新的 if-else 分支
4. 重新编译即可

### Q: 编译报错怎么办？

常见问题：

**Windows（MSVC）：**
- 找不到 `<winsock2.h>`：确认使用 MSVC 编译器，并在 Visual Studio Developer Command Prompt 中编译
- 链接错误：确认命令末尾有 `ws2_32.lib`

**macOS（g++/clang）：**
- 找不到 `<sys/socket.h>`：macOS 自带 POSIX socket 头文件，确认使用 g++ 或 clang++ 编译
- `fatal error: 'mach-o/dyld.h' file not found`：确认在 macOS 上编译（这是 macOS 特有头文件）

**Linux（g++）：**
- 找不到 `<sys/socket.h>`：确认安装了 build-essential（`sudo apt install build-essential`）
- 没有 `/proc/self/exe`：确认在标准 Linux 发行版上运行

**通用：**
- C++11 特性不支持：确认编译器版本（VS2013+、GCC 4.8+、Clang 3.3+）
- 中文乱码：Windows 用 `/utf-8` 标志编译，macOS/Linux 默认 UTF-8

---

## 学习建议

如果你是 C++ 初学者，建议按以下顺序阅读理解代码：

1. **先看 `utils.h` / `utils.cpp`** — 理解基础工具函数的写法（哈希、字符串处理、JSON字符串拼接）
2. **再看 `sort.h` / `sort.cpp`** — 理解手写的冒泡、选择、快速排序三种算法
3. **然后看 `storage.h` / `storage.cpp`** — 理解 fstream 文件读写和 CSV 格式解析
4. **接着看 `models.h` / `models.cpp`** — 理解 OOP 的类定义、封装、继承（Person→Admin/Student）
5. **再接着看 `handlers.h` / `handlers.cpp`** — 理解真实的 CRUD 业务逻辑怎么写
6. **然后看 `stats.h` / `stats.cpp`** — 理解统计分析算法（排名、对比、分布、预警）
7. **再看 `server.h` / `server.cpp`** — 理解 WinSock 网络编程（Socket、bind、listen、accept）
8. **最后看 `main.cpp`** — 理解程序入口、路由分发、种子数据生成

每个文件都使用初学者友好的写法：没有 lambda 表达式、没有模板、没有智能指针、没有虚函数。所有代码都有详细的中文注释，建议一边看代码一边看注释。
