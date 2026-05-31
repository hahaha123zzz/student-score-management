# EduGrade

*C++ 后端 | 高中学生成绩管理系统*

一个基于 C++17 后端 + HTML/CSS/JS 前端的高中学生成绩管理系统，支持多角色登录、成绩录入与管理、统计分析、错题追踪和智能学习建议。

## 功能特性

### 用户与权限
- **多角色体系** — 管理员、教师、学生、家长四种角色，各具不同权限
- **JWT 认证** — 基于 Token 的安全登录与身份校验
- **密码管理等** — 支持修改密码、用户增删改查

### 学生与班级管理
- **学生档案** — 学号、姓名、性别、生日、班级、家长绑定
- **班级管理** — 年级-班级两级结构
- **搜索与筛选** — 按姓名、班级快速检索学生

### 成绩管理
- **成绩录入** — 单条录入或 CSV 批量导入
- **成绩锁定** — 发布后锁定，防止篡改
- **补考管理** — 支持补考成绩录入
- **成绩导出** — 导出为 CSV 格式

### 统计分析
- **排名统计** — 总分排名 / 单科排名
- **班级对比** — 各班平均分、优秀率等横向对比
- **成绩分布** — 各科目分数段分布直方图
- **趋势分析** — 学生历次考试成绩变化趋势图
- **偏科分析** — 检测学生各科成绩不均衡程度
- **能力评估** — 基于考试数据的能力维度分析

### 智能分析
- **错题本** — 自动追踪学生错题，支持按考试/科目筛选
- **错题统计** — 错题频率与薄弱知识点分析
- **学习报告** — 一键生成学生综合学习报告
- **学习建议** — 基于数据分析的个性化学习建议

### 前端体验
- **响应式设计** — 适配桌面与移动端
- **Chart.js 可视化** — 成绩趋势、分布图等交互式图表
- **单页面应用** — 管理员看板、学生看板、家长看板

---

## 技术栈

| 层级 | 技术 | 版本 / 说明 |
|------|------|------------|
| **后端语言** | C++ | C++17 标准 |
| **HTTP 库** | cpp-httplib | 单头文件 HTTP 服务器 |
| **JSON 库** | nlohmann/json | 现代 C++ JSON 解析 |
| **数据存储** | JSON 文件 | data/ 目录下结构化存储 |
| **前端** | HTML5 | 语义化标签 |
| **样式** | CSS3 | Flexbox / Grid 布局 |
| **交互** | JavaScript (ES6+) | Fetch API + 动态渲染 |
| **图表** | Chart.js | 交互式数据可视化 |

---

## 项目结构

```
edugrade/
├── CMakeLists.txt          # CMake 构建配置
├── README.md
├── edugrade.exe            # 编译可执行文件
├── include/                # 头文件
│   ├── util.h              #   工具函数（哈希、时间、JWT、文件读写）
│   ├── auth.h              #   用户认证与权限管理
│   ├── student.h           #   学生与班级管理
│   ├── exam.h              #   考试与科目管理
│   ├── grade.h             #   成绩管理
│   ├── stats.h             #   统计分析
│   └── extra.h             #   错题本与智能分析
├── src/                    # 源文件
│   ├── main.cpp            #   入口 + 路由注册 + 种子数据
│   ├── util.cpp
│   ├── auth.cpp
│   ├── student.cpp
│   ├── exam.cpp
│   ├── grade.cpp
│   ├── stats.cpp
│   └── extra.cpp
├── lib/                    # 第三方库
│   ├── httplib.h           #   cpp-httplib (header-only)
│   └── json.hpp            #   nlohmann/json (header-only)
├── data/                   # 数据文件
│   ├── users.json          #   用户账户
│   ├── students.json       #   学生档案
│   ├── classes.json        #   班级信息
│   ├── subjects.json       #   科目设置
│   ├── exams.json          #   考试记录
│   ├── grades.json         #   成绩数据
│   └── logs.json           #   操作日志
└── web/                    # 前端页面
    ├── index.html          #   登录页
    ├── admin.html          #   管理员看板
    ├── student.html         #   学生看板
    ├── parent.html         #   家长看板
    ├── css/
    │   └── style.css       #   全局样式
    └── js/
        ├── api.js          #   API 封装层
        └── common.js       #   公共工具函数
```

---

## 快速开始

### 环境要求

- **操作系统**: Windows (WinSock2 依赖)
- **编译器**: Visual Studio 2019+ (MSVC) 或其他支持 C++17 的编译器
- **浏览器**: 任意现代浏览器（Chrome / Edge / Firefox）

### 编译

**方式一：使用 CMake**

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

**方式二：直接使用 MSVC 编译器**

```bash
cl /EHsc /std:c++17 /I include /I lib ^
   src/main.cpp src/util.cpp src/auth.cpp src/student.cpp ^
   src/exam.cpp src/grade.cpp src/stats.cpp src/extra.cpp ^
   /Fe:edugrade.exe /link ws2_32.lib
```

### 运行

```bash
.\edugrade.exe
```

启动后在浏览器打开 **http://localhost:8080** 即可访问系统。

### 默认账户

| 角色 | 用户名 | 密码 | 说明 |
|------|--------|------|------|
| 系统管理员 | `admin` | `123456` | 全部管理权限 |
| 教师 | `T001` | `123456` | 成绩录入、考试管理 |
| 学生 | `S2024001` | `123456` | 查看成绩、错题、报告 |
| 家长 | `P2024001` | `123456` | 绑定学生 S2024001 |

---

## API 文档概览

Base URL: `http://localhost:8080/api`

### 认证模块 `/api`

| 方法 | 端点 | 说明 |
|------|------|------|
| POST | `/login` | 用户登录，返回 Token |
| GET | `/auth/check` | 验证 Token 有效性 |
| POST | `/change-password` | 修改密码 |

### 用户管理 `/api/users`

| 方法 | 端点 | 说明 |
|------|------|------|
| GET | `/users` | 获取用户列表 |
| POST | `/users` | 添加用户 |
| PUT | `/users/:id` | 更新用户信息 |
| DELETE | `/users/:id` | 删除用户 |

### 学生管理 `/api/students`

| 方法 | 端点 | 说明 |
|------|------|------|
| GET | `/students` | 学生列表（支持搜索、分页） |
| POST | `/students` | 添加学生 |
| PUT | `/students/:id` | 更新学生信息 |
| DELETE | `/students/:id` | 删除学生 |

### 班级管理 `/api/classes`

| 方法 | 端点 | 说明 |
|------|------|------|
| GET | `/classes` | 班级列表 |
| POST | `/classes` | 添加班级 |
| PUT | `/classes/:id` | 更新班级 |
| DELETE | `/classes/:id` | 删除班级 |

### 科目管理 `/api/subjects`

| 方法 | 端点 | 说明 |
|------|------|------|
| GET | `/subjects` | 科目列表 |
| POST | `/subjects` | 添加科目 |

### 考试管理 `/api/exams`

| 方法 | 端点 | 说明 |
|------|------|------|
| GET | `/exams` | 考试列表（支持状态筛选） |
| POST | `/exams` | 创建考试 |
| PUT | `/exams/:id` | 更新考试信息 |
| DELETE | `/exams/:id` | 删除考试 |
| PUT | `/exams/:id/publish` | 发布考试成绩 |
| PUT | `/exams/:id/retract` | 撤回考试成绩 |

### 成绩管理 `/api/grades`

| 方法 | 端点 | 说明 |
|------|------|------|
| GET | `/grades?exam_id=` | 按考试获取成绩列表 |
| GET | `/grades/student/:id` | 获取学生所有成绩 |
| POST | `/grades` | 录入单条成绩 |
| POST | `/grades/import` | CSV 批量导入成绩 |
| GET | `/grades/export?exam_id=` | 导出成绩为 CSV |
| PUT | `/grades/:exam_id/lock` | 锁定考试成绩 |
| POST | `/grades/makeup` | 录入补考成绩 |

### 统计分析 `/api/stats`

| 方法 | 端点 | 说明 |
|------|------|------|
| GET | `/stats/rank` | 排名统计 |
| GET | `/stats/class-compare` | 班级对比分析 |
| GET | `/stats/distribution` | 成绩分布 |
| GET | `/stats/trend/:student_id` | 学生成绩趋势 |
| GET | `/stats/warnings` | 警告信息 |
| GET | `/stats/enrollment` | 入学统计 |
| GET | `/stats/imbalance` | 偏科分析 |
| GET | `/stats/ability` | 能力评估 |

### 智能分析 `/api/`

| 方法 | 端点 | 说明 |
|------|------|------|
| GET | `/report?student_id=` | 获取学习报告 |
| GET | `/suggest?student_id=` | 获取学习建议 |
| POST | `/errors` | 记录错题 |
| GET | `/errors` | 查询错题（支持筛选） |
| GET | `/errors/stats?student_id=` | 错题统计 |

### 日志 `/api/logs`

| 方法 | 端点 | 说明 |
|------|------|------|
| GET | `/logs` | 操作日志（分页） |

---

## 角色说明

| 角色 | 权限范围 |
|------|---------|
| **管理员** | 用户管理、班级管理、科目管理、考试管理、成绩管理、统计分析、全部数据的增删改查 |
| **学生** | 查看个人成绩与排名、成绩趋势图表、错题本、学习报告与建议 |
| **家长** | 查看绑定学生的成绩、趋势图表、学习报告；不能修改数据 |

---

## 界面预览

> *（截图占位 — 运行时访问 http://localhost:8080 体验）*

- **登录页** — 角色选择与账号密码登录
- **管理员看板** — 数据总览、用户/学生/考试管理面板
- **学生看板** — 成绩趋势图、错题列表、学习建议
- **家长看板** — 绑定学生成绩与报告查看

---

## 作者

**EduGrade** — 软件工程课程设计项目

---

## 许可证

MIT License
