# EduGrade

*C++11 后端 | 高中学生成绩管理系统课程作业版*

这是一个面向大一课程设计整理后的成绩管理系统。项目保留了课程作业真正需要的核心能力：

- 两种角色：管理员、学生
- 管理员管理学生、科目、考试、成绩
- 学生查看自己的成绩、班级排名、年级排名，并修改个人信息
- 自动计算总分、平均分
- 支持按总分排序、按单科排序
- 使用 `fstream` 做文件持久化
- 保留网页界面，并沿用原有页面样式体系做精简改造

当前版本已经去掉了原项目里超出课程作业范围的扩展模块，例如家长端、错题本、学习建议、成长档案、积分系统等。

---

## 技术说明

### 后端实现约束

- **C++ 标准**：`C++11`
- **网络层例外**：`cpp-httplib`
- **其余部分**：只使用 C++ 标准库
- **不使用自定义业务头文件**
- **数据存储**：`data/*.txt`

也就是说，除了 HTTP 服务使用 `httplib.h` 之外，业务代码全部基于下面这些标准库能力实现：

- `string`
- `vector`
- `map`
- `set`
- `fstream`
- `sstream`
- `algorithm`
- `ctime`

### 前端实现约束

- 继续使用原有 `web/` 页面结构
- 继续使用原有 `web/css/style.css` 和 `web/css/home.css`
- 页面只做功能收缩和数据绑定调整，不重做一套新样式

---

## 代码拆分说明

为了让代码更容易看懂，同时继续满足“不要自定义头文件”的要求，后端不再写一大块 `main.cpp`，而是拆成 **5 个部分**：

1. [src/core_part1_base.cpp](/C:/Users/29802/Documents/student-score-management/src/core_part1_base.cpp)
   负责头文件、基础数据结构、字符串工具、表单解析、JSON 字符串拼接。

2. [src/core_part2_storage.cpp](/C:/Users/29802/Documents/student-score-management/src/core_part2_storage.cpp)
   负责文本数据解析、文件读写、示例数据初始化。

3. [src/core_part3_logic.cpp](/C:/Users/29802/Documents/student-score-management/src/core_part3_logic.cpp)
   负责查找函数、权限校验、日志写入、总分/平均分/排名计算、JSON 序列化。

4. [src/core_part4_handlers.cpp](/C:/Users/29802/Documents/student-score-management/src/core_part4_handlers.cpp)
   负责各个 API 接口处理函数。

5. [src/main.cpp](/C:/Users/29802/Documents/student-score-management/src/main.cpp)
   只负责组合前 4 个部分、注册路由和启动服务器。

这种写法虽然仍然是 `.cpp` 之间的组合，但对课程作业很实用：

- 不需要额外写 `.h`
- 阅读顺序更清楚
- 每一部分职责更单一

---

## 当前功能

### 1. 用户与权限

- 管理员登录
- 学生登录
- 修改密码
- 学生只能访问自己的数据
- 管理员可以访问和维护全部核心数据

### 2. 学生管理

- 添加学生
- 修改学生
- 删除学生
- 按学号或姓名搜索
- 按班级筛选
- 学生修改自己的姓名、性别、生日、电话

### 3. 科目管理

- 添加科目
- 修改科目名称和满分
- 删除未被考试使用的科目

### 4. 考试管理

- 添加考试
- 修改考试名称、日期、科目
- 删除考试

### 5. 成绩管理

- 录入单条成绩
- 修改成绩
- 删除成绩
- 按考试查看成绩
- 按班级筛选某场考试成绩
- 缺考记为 `ABS`，前端显示为“缺考”

### 6. 排名与统计

- 自动计算总分
- 自动计算平均分
- 年级排名
- 班级排名
- 按总分排序
- 按单科排序
- 考试统计概览
- 成绩导出为 CSV

---

## 数据文件

当前项目的数据全部放在 `data/` 目录下，使用简单文本格式保存：

- `users.txt`
- `students.txt`
- `subjects.txt`
- `exams.txt`
- `grades.txt`
- `logs.txt`

### 记录格式示例

`users.txt`

```txt
admin|系统管理员|admin|123456|2026-02-20 09:00:00
S2024001|张三|student|123456|2026-02-20 09:00:00
```

`students.txt`

```txt
S2024001|张三|男|2008-01-12|高一1班||2026-02-20 09:00:00
```

`grades.txt`

```txt
EXAM1|S2024001|数学:95,英语:100,语文:98|2026-03-21 18:00:00
```

这种格式的优点是：

- 容易用 `fstream` 读写
- 容易人工检查数据
- 非常适合作业答辩时说明“文件持久化”的实现方式

---

## 项目结构

```txt
student-score-management/
├── CMakeLists.txt
├── README.md
├── edugrade.exe
├── lib/
│   └── httplib.h
├── src/
│   ├── core_part1_base.cpp
│   ├── core_part2_storage.cpp
│   ├── core_part3_logic.cpp
│   ├── core_part4_handlers.cpp
│   └── main.cpp
├── data/
│   ├── users.txt
│   ├── students.txt
│   ├── subjects.txt
│   ├── exams.txt
│   ├── grades.txt
│   └── logs.txt
└── web/
    ├── index.html
    ├── login.html
    ├── admin.html
    ├── student.html
    ├── home.html
    ├── parent.html
    ├── css/
    │   ├── style.css
    │   └── home.css
    └── js/
        ├── api.js
        ├── common.js
        ├── admin.js
        └── student.js
```

---

## 快速开始

### 环境要求

- Windows
- 支持 C++11 的编译器
- CMake
- 现代浏览器

### 编译

```bash
cmake -S . -B build
cmake --build build
```

编译完成后会在项目根目录生成：

```txt
edugrade.exe
```

### 运行

```bash
.\edugrade.exe
```

启动后在浏览器打开：

```txt
http://localhost:8080
```

---

## 默认账号

| 角色 | 用户名 | 密码 |
|------|--------|------|
| 管理员 | `admin` | `123456` |
| 学生 | `S2024001` | `123456` |
| 学生 | `S2024002` | `123456` |
| 学生 | `S2024003` | `123456` |

---

## 主要接口

Base URL:

```txt
http://localhost:8080/api
```

### 认证

- `POST /login`
- `GET /auth/check`
- `POST /change-password`

### 学生

- `GET /students`
- `POST /students`
- `GET /students/:id`
- `PUT /students/:id`
- `DELETE /students/:id`

### 科目

- `GET /subjects`
- `POST /subjects`
- `PUT /subjects/:id`
- `DELETE /subjects/:id`

### 考试

- `GET /exams`
- `POST /exams`
- `PUT /exams/:id`
- `DELETE /exams/:id`

### 成绩

- `GET /grades`
- `POST /grades`
- `DELETE /grades/:exam_id/:student_id`
- `GET /grades/student/:id`

### 排名与统计

- `GET /rank/grade`
- `GET /rank/class`
- `GET /stats`
- `GET /export`

---

## 阅读建议

如果你现在要自己读懂代码，建议按下面顺序看：

1. 先看 [src/main.cpp](/C:/Users/29802/Documents/student-score-management/src/main.cpp)
   先弄清楚程序启动流程和接口分组。

2. 再看 [src/core_part1_base.cpp](/C:/Users/29802/Documents/student-score-management/src/core_part1_base.cpp)
   先认识项目里有哪些结构体、工具函数和公共能力。

3. 再看 [src/core_part2_storage.cpp](/C:/Users/29802/Documents/student-score-management/src/core_part2_storage.cpp)
   重点理解文件读写和文本格式。

4. 再看 [src/core_part3_logic.cpp](/C:/Users/29802/Documents/student-score-management/src/core_part3_logic.cpp)
   重点理解总分、平均分、班级排名、年级排名。

5. 最后看 [src/core_part4_handlers.cpp](/C:/Users/29802/Documents/student-score-management/src/core_part4_handlers.cpp)
   重点理解“前端请求是如何调用业务逻辑的”。

---

## 说明

- 当前版本是课程作业导向版本，不追求复杂架构
- 后端逻辑优先追求“能看懂、能解释、能答辩”
- 页面样式继续沿用原项目视觉体系，只删掉与作业无关的扩展功能
