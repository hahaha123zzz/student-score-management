# 学生成绩管理系统

> 一套用 C++ 从零构建的 Web 成绩管理系统，适合大一课程设计 / C++ 入门学习。

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

这是一个**基于 C++ 编写的 HTTP 服务器**，提供完整的学生成绩管理功能。你可以通过浏览器访问网页界面，进行以下操作：

- 学生档案管理（增删改查）
- 班级管理
- 科目管理
- 考试管理（创建、发布、撤回、锁定）
- 成绩录入（手动录入 + CSV 批量导入导出）
- 补考成绩管理
- 统计分析（排名、班级对比、分数段分布、趋势）
- 不及格预警
- 操作日志

**第一次运行时会自动生成 50 名学生的测试数据**，可以直接用来演示和测试。

---

## 技术栈

| 层     | 技术                             | 说明                   |
| ------ | -------------------------------- | ---------------------- |
| 后端   | C++17                            | 编程语言               |
| HTTP   | cpp-httplib (httplib.h)          | 单头文件 HTTP 服务器库 |
| JSON   | nlohmann/json (json.hpp)         | JSON 解析库            |
| 存储   | 本地 JSON 文件 (`data/` 目录)    | 无需数据库             |
| 前端   | 原生 HTML + CSS                  | 无需前端框架           |
| 构建   | CMake（推荐）/ 直接编译          | 跨平台                 |

**两个第三方库都是 header-only（只需头文件，不用单独编译）**，放进 `lib/` 目录就能用。

---

## 快速开始

### 方式一：直接运行（Windows）

如果你已经有编译好的 `edugrade.exe`，直接双击运行，然后在浏览器打开 **http://localhost:8080**。

### 方式二：用 CMake 编译

```bash
# 1. 创建构建目录
mkdir build
cd build

# 2. 生成工程（Visual Studio / MinGW / GCC 都可以）
cmake ..

# 3. 编译
cmake --build . --config Release

# 4. 运行
./Release/edugrade.exe
```

### 方式三：手动编译（适合小项目）

```bash
# Windows (MSVC)
cl /std:c++17 /EHsc /Fe:edugrade.exe src/*.cpp /I include /I lib ws2_32.lib

# Linux / Mac
g++ -std=c++17 -o edugrade src/*.cpp -I include -I lib -lpthread
```

运行后在浏览器访问 **http://localhost:8080/login.html** 进入登录页。

---

## 项目结构

```
c++成绩管理系统/
├── lib/                          # 第三方库（只用头文件）
│   ├── httplib.h                 # cpp-httplib：HTTP 服务器库
│   └── json.hpp                  # nlohmann/json：JSON 解析库
│
├── include/                      # 头文件（函数声明）
│   ├── util.h                    # 工具模块
│   ├── auth.h                    # 认证模块
│   ├── student.h                 # 学生管理模块
│   ├── exam.h                    # 考试管理模块
│   ├── grade.h                   # 成绩管理模块
│   └── stats.h                   # 统计分析模块
│
├── src/                          # 源文件（函数实现）
│   ├── main.cpp                  # 程序入口 + HTTP 路由注册
│   ├── util.cpp                  # 工具函数实现
│   ├── auth.cpp                  # 认证功能实现
│   ├── student.cpp               # 学生/班级管理实现
│   ├── exam.cpp                  # 考试管理实现
│   ├── grade.cpp                 # 成绩管理实现
│   └── stats.cpp                 # 统计分析实现
│
├── web/                          # 前端页面
│   ├── login.html                # 登录页
│   ├── admin.html                # 管理员后台
│   ├── student.html              # 学生中心
│   └── css/style.css             # 样式表
│
├── data/                         # 数据文件（运行时生成）
│   ├── users.json                # 用户账号
│   ├── students.json             # 学生档案
│   ├── classes.json              # 班级信息
│   ├── subjects.json             # 科目设置
│   ├── exams.json                # 考试记录
│   ├── grades.json               # 成绩数据
│   └── logs.json                 # 操作日志
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

### 4. JSON 是什么？

JSON 是一种数据格式，和 XML 类似但更简洁。它看起来像这样：

```json
{
    "name": "张三",
    "age": 16,
    "scores": { "语文": 120, "数学": 135 }
}
```

所有数据（学生、成绩、用户等）都以 JSON 文件格式存储在 `data/` 目录下。

### 5. 为什么用 JSON 文件而不是数据库？

对于课程设计级别的项目：
- JSON 文件**简单直观**，可以直接用记事本打开查看
- 不需要安装和配置数据库
- 不需要学习 SQL 语言
- 数据量小（几百个学生）时性能完全够用

在实际项目中，会用 MySQL / SQLite / MongoDB 等数据库替换这种方式。

### 6. 哈希是什么？为什么要哈希存密码？

哈希就像"数字指纹"：`hasher("123456")` 永远等于 `"150a41"`，但你无法从 `"150a41"` 反推出原始密码是 `123456`。

这样即使有人拿到了 users.json 文件，也看不到真实密码。

---

## 代码架构

整个项目分为 **6 个模块**，每个模块一个头文件 + 一个源文件：

```
main.cpp  ← 入口，注册所有路由
  ├── util.h / util.cpp        ← 基础工具（其他模块都依赖它）
  ├── auth.h / auth.cpp        ← 登录认证 + 用户管理
  ├── student.h / student.cpp  ← 学生 + 班级 + 科目管理
  ├── exam.h / exam.cpp        ← 考试管理
  ├── grade.h / grade.cpp      ← 成绩录入查询导入导出
  └── stats.h / stats.cpp      ← 统计分析
```

### 模块依赖关系

```
main.cpp ──→ student ──→ util
         ──→ auth    ──→ util
         ──→ exam    ──→ util
         ──→ grade   ──→ util
         ──→ stats   ──→ util
```

**util 模块是基础层**，所有其他模块都调用它来读写文件、记录日志、生成响应。

### 程序的执行流程

```
main() 被调用
  │
  ├─ ① seedData()         → 检查并生成测试数据
  │
  ├─ ② 创建 Server 对象   → 初始化 HTTP 服务器
  │
  ├─ ③ 注册路由           → 告诉服务器"什么 URL 调什么函数"
  │     ├─ POST /api/login         → auth::login()
  │     ├─ GET  /api/students      → student::getStudents()
  │     ├─ POST /api/grades        → grade::setGrades()
  │     └─ ... （共约 30 个路由）
  │
  ├─ ④ svr.set_mount_point("/", web/)  → 设置前端文件目录
  │
  └─ ⑤ svr.listen("0.0.0.0", 8080)    → 开始监听，等待请求
       │
       └─ 用户访问 http://localhost:8080/login.html
            └─ 服务器返回 web/login.html 的内容
            └─ 用户填写表单，点登录
            └─ 浏览器 POST /api/login
            └─ 服务器调用 auth::login()
            └─ 返回 Token 给浏览器
            └─ 浏览器存下 Token，后续请求都带着它
```

### 一个请求的完整处理过程

以"查看学生列表"为例：

```
浏览器 GET /api/students?page=1&size=20
    │
    ▼
[httplib 库收到请求]
    │
    ▼
[匹配路由]  → GET /api/students
    │
    ▼
[执行 lambda]  {
    addCors(res);                              ← 加跨域头
    parseParams(req, page, size);               ← 解析 URL 参数
    student::getStudents(search, cls, 1, 20)    ← 调用业务逻辑
        ├── util::readJSON("students.json")     ← 读文件
        ├── 筛选 + 分页                          ← 处理数据
        └── return json 结果                     ← 返回
    res.set_content(result.dump(), "application/json")  ← 发送响应
}
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
| GET    | `/api/users`      | 用户列表     | 管理员 |
| POST   | `/api/users`      | 添加用户     | 管理员 |
| PUT    | `/api/users/{id}` | 修改用户     | 管理员 |
| DELETE | `/api/users/{id}` | 删除用户     | 管理员 |

### 学生管理

| 方法   | URL                  | 说明             | 权限   |
| ------ | -------------------- | ---------------- | ------ |
| GET    | `/api/students`      | 学生列表（分页） | 所有人 |
| POST   | `/api/students`      | 添加学生         | 管理员 |
| PUT    | `/api/students/{id}` | 修改学生         | 管理员 |
| DELETE | `/api/students/{id}` | 删除学生         | 管理员 |

### 班级管理

| 方法   | URL                 | 说明     | 权限   |
| ------ | ------------------- | -------- | ------ |
| GET    | `/api/classes`      | 班级列表 | 所有人 |
| POST   | `/api/classes`      | 添加班级 | 管理员 |
| PUT    | `/api/classes/{id}` | 修改班级 | 管理员 |
| DELETE | `/api/classes/{id}` | 删除班级 | 管理员 |

### 科目管理

| 方法 | URL              | 说明     | 权限   |
| ---- | ---------------- | -------- | ------ |
| GET  | `/api/subjects`  | 科目列表 | 所有人 |
| POST | `/api/subjects`  | 添加科目 | 管理员 |

### 考试管理

| 方法   | URL                       | 说明       | 权限   |
| ------ | ------------------------- | ---------- | ------ |
| GET    | `/api/exams`              | 考试列表   | 所有人 |
| POST   | `/api/exams`              | 创建考试   | 管理员 |
| PUT    | `/api/exams/{id}`         | 修改考试   | 管理员 |
| DELETE | `/api/exams/{id}`         | 删除考试   | 管理员 |
| PUT    | `/api/exams/{id}/publish` | 发布成绩   | 管理员 |
| PUT    | `/api/exams/{id}/retract` | 撤回成绩   | 管理员 |

### 成绩管理

| 方法   | URL                         | 说明             | 权限   |
| ------ | --------------------------- | ---------------- | ------ |
| POST   | `/api/grades`               | 录入/更新成绩    | 管理员 |
| POST   | `/api/grades/import`        | CSV 批量导入成绩 | 管理员 |
| GET    | `/api/grades/export`        | CSV 导出成绩     | 所有人 |
| PUT    | `/api/grades/{id}/lock`     | 锁定成绩         | 管理员 |
| GET    | `/api/grades`               | 查询成绩         | 所有人 |
| GET    | `/api/grades/student/{id}`  | 学生历史成绩     | 所有人 |
| POST   | `/api/grades/makeup`        | 补考成绩录入     | 管理员 |

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

| 方法 | URL        | 说明               | 权限   |
| ---- | ---------- | ------------------ | ------ |
| GET  | `/api/logs` | 操作日志（需要管理员）| 管理员 |

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

### util（工具模块）— `util.h` / `util.cpp`

**定位：** 最底层的工具箱，所有模块都依赖它。

**主要功能：**
- `readJSON / writeJSON` — JSON 文件的读写（自动创建目录）
- `hasher` — DJB2 哈希算法，用于密码加密存储
- `genToken` — 生成 32 位随机 Token
- `logAction / getLogs` — 操作日志的记录和查询
- `getCurrentTime` — 获取格式化时间字符串
- `round2` — 保留两位小数
- `gradeToRank` — 分数 → 等级（A/B/C/D/F）+ GPA
- `jsonResponse / errorResponse` — 统一 API 响应格式
- `extractToken` — 从 HTTP 请求头提取 Token

### auth（认证模块）— `auth.h` / `auth.cpp`

**定位：** 管理用户身份和权限。

**主要功能：**
- `login` — 验证用户名密码，返回 Token
- `checkAuth` — 验证 Token 有效性
- `changePassword` — 修改密码
- `getUsers / addUser / deleteUser / updateUser` — 用户 CRUD

**核心数据结构：** `sessions`（内存中的 Token → 用户映射表）

**注意：** 重启服务器后 session 会清空，用户需要重新登录。

### student（学生管理模块）— `student.h` / `student.cpp`

**定位：** 管理学生、班级、科目的基础数据。

**主要功能：**
- 学生 CRUD（支持姓名/学号搜索 + 班级筛选 + 分页）
- 班级 CRUD
- 科目管理
- 入学统计（每个班级多少人）

### exam（考试管理模块）— `exam.h` / `exam.cpp`

**定位：** 管理考试的生命周期。

**状态机：**
```
draft ──publish──→ published ──retract──→ draft
  │                    │
  └──lock──→ locked    └──lock──→ locked
```

- `draft`：草稿，只有管理员能看到
- `published`：已发布，学生可以在自己页面看到成绩
- `locked`：已锁定，不能修改、不能发布、不能撤回（最终封存）

### grade（成绩管理模块）— `grade.h` / `grade.cpp`

**定位：** 成绩的核心增删改查。

**主要功能：**
- `setGrades` — 录入/覆盖成绩（自动判断新增还是更新）
- `importCSV` — 批量导入（解析 CSV 格式）
- `exportCSV` — 批量导出（生成 CSV 格式）
- `lockGrades` — 锁定成绩
- `getGrades` — 查询（支持按考试、按学生筛选）
- `markMakeup` — 补考成绩标注

**CSV 导入格式示例：**
```csv
学号,姓名,语文,数学,英语
S2024001,张三,120,135,110
S2024002,李四,115,128,105
```

### stats（统计分析模块）— `stats.h` / `stats.cpp`

**定位：** 对成绩数据进行深度分析。

**主要功能：**
- `getRank` — 排名（总分/单科，全年级/班级内）
- `getClassCompare` — 班级对比（平均分、优秀率、及格率、中位数等）
- `getDistribution` — 分数段分布（<60, 60-69, 70-79, 80-89, 90-100）
- `getTrend` — 学生成绩趋势（多场考试的变化）
- `getWarnings` — 不及格预警
- `convertGrade` — 加权总分换算

### main.cpp（程序入口）

**定位：** 程序的"大脑"，负责串联一切。

**做的事情：**
1. `seedData()` — 初始化测试数据
2. 创建 HTTP 服务器
3. 注册 ~30 个 API 路由
4. 设置前端静态文件目录
5. 启动服务器监听端口 8080

---

## 常见问题

### Q: 怎么改端口号？

在 `src/main.cpp` 最后一行，把 `8080` 改成你想要的端口号：

```cpp
svr.listen("0.0.0.0", 8080);  // 改成 3000 或其他
```

### Q: 数据保存在哪里？

所有数据都保存在 `data/` 目录下的 JSON 文件中。可以直接用记事本打开查看。

### Q: 怎么重置所有数据？

删除 `data/` 目录，重新运行程序，它会自动生成全新的测试数据。

### Q: 为什么密码看不到？

密码存储的是哈希值（数字指纹），不是明文。这是安全设计——即使数据文件泄露，也无法知道真实密码。

### Q: 程序支持多个用户同时访问吗？

是的，服务器会为每个请求创建线程处理，支持多人同时操作。

### Q: 前端页面怎么自己修改？

前端页面在 `web/` 目录下，是纯 HTML + CSS，可以直接编辑。修改后刷新浏览器即可看到效果，不需要重新编译。

### Q: 怎么添加新功能？

以添加一个"导出排名为 Excel"的功能为例：
1. 在 `src/stats.cpp` 中写一个新函数（如 `exportRankExcel`）
2. 在 `include/stats.h` 中声明它
3. 在 `src/main.cpp` 中注册一个新的路由
4. 重新编译即可

### Q: 编译报错怎么办？

常见问题：
- Windows 上缺少 `ws2_32.lib`：CMakeLists.txt 已经配置好了，用 CMake 编译即可
- 找不到 `httplib.h` 或 `json.hpp`：确认 `lib/` 目录下这两个文件存在
- C++17 特性不支持：确认编译器版本够新（VS2017+、GCC 8+）

---

## 学习建议

如果你是 C++ 初学者，建议按以下顺序阅读理解代码：

1. **先看 `util.h` / `util.cpp`** — 理解基础工具函数的写法
2. **再看 `auth.h` / `auth.cpp`** — 理解登录认证流程和 Token 机制
3. **然后看 `student.h` / `student.cpp`** — 理解 CRUD 的标准写法
4. **接着看 `main.cpp`** — 理解 HTTP 路由注册和服务器启动
5. **最后看 `stats.cpp`** — 理解较复杂的统计算法

每个文件都已经添加了非常详细的注释，建议一边看代码一边看注释，遇到不理解的 C++ 语法可以查阅课本或搜索引擎。
