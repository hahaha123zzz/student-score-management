# 成绩管理系统优化方案（大白话版）

## 咱们的情况

- 5 个大一学生，刚学完 C++ 基础
- 会用：class、fstream 读写文件、vector、string、继承、排序
- 不会：前端（HTML/CSS/JS）、网络编程、CMake
- 项目要求：C++ 做后端 + 用 class 设计 + 文件存数据 + 排序 + 两个角色登录
- Web 前端是**加分项**，不是必须的，但做了能多拿分
- CMake 要留着，因为要在 Windows 和 Mac 上都能跑

## 咱们要做什么

现在这个项目是别人写的，里面有很多咱们不需要的东西，也有很多咱们看不懂的写法。咱们要在它的基础上**删东西 + 改写法**，变成咱们能看懂、能交作业的版本。

---

## 一、删掉不需要的代码

### 删掉 2 个 C++ 文件（后端）

这两个文件做的都是课程没有要求的功能，删掉：

| 删掉的文件 | 里面是什么 | 为什么删 |
|-----------|-----------|---------|
| `src/enhance.cpp` + `include/enhance.h` | 积分系统、打卡签到、复习计划 | 课程没要求 |
| `src/extra.cpp` + `include/extra.h` | 错题本、学习建议、雷达图分析 | 课程没要求 |

删完以后，项目从 9 个 .cpp 文件变成 7 个，清爽多了。

### 删掉 4 个数据文件

这些 JSON 文件是上面两个模块用的，一起删掉：

- `data/errors.json`（错题本数据）
- `data/goals.json`（学习目标数据）
- `data/points.json`（积分数据）
- `data/checkins.json`（打卡数据）

### 删掉 3 个网页

| 删掉的文件 | 为什么 |
|-----------|--------|
| `web/index.html` | 没啥用，打开直接跳登录就行 |
| `web/home.html` | 功能可以合并到后面两个页面里 |
| `web/parent.html` | 课程只要管理员和学生，不要家长 |

### 删掉前端的复杂依赖

- 扔掉 Chart.js（画图表的，大一不需要学）
- 删掉 `web/js/api.js` 和 `web/js/common.js`（太复杂的 JS）

---

## 二、重写 3 个网页（E 同学负责）

只做 3 个最简单的页面，每个页面就是一个 HTML 文件加一点 CSS：

### 1. 登录页（login.html）
就是一个表单，有三个输入框 + 一个按钮：
```
用户名：[        ]
密码：  [        ]
角色：  [管理员] [学生]   ← 两个按钮选一个
         [ 登录 ]
```
知识点：HTML 的 `<form>` + `<input>` + `<button>`
JS 代码：就一行 fetch 发请求到 `/api/login`

### 2. 管理员页（admin.html）
上面几个标签按钮切换功能，下面显示内容：
```
[学生管理] [成绩录入] [成绩查询] [统计排名]

学生管理：表格显示所有学生，可以增删改
成绩录入：选考试 → 选学生 → 输入各科分数
成绩查询：表格显示成绩，可以按总分排序
统计排名：班级排名表、年级排名表
```
知识点：HTML 的 `<table>`（表格）、`<select>`（下拉框）
JS 代码：几个 fetch 请求拿数据，用 innerHTML 画表格

### 3. 学生页（student.html）
```
个人信息：[姓名] [班级]    [修改]
我的成绩：表格显示各科分数、总分、平均分
我的排名：班级第X名 / 年级第X名
```
知识点：和上面一样
JS 代码：三四个 fetch 请求拿数据

### 前端同学只需要学这些

| 要学的东西 | 多久学会 | 一句话解释 |
|-----------|---------|-----------|
| `<table>` 表格 | 10 分钟 | 像 Excel 一样画表格 |
| `<form>` 表单 | 10 分钟 | 做输入框和按钮 |
| CSS 简单美化 | 30 分钟 | 让页面不丑（给个模板抄就行） |
| fetch 发请求 | 30 分钟 | 前端问后端要数据 |
| innerHTML | 15 分钟 | 把数据变成表格显示出来 |
| **总共** | **约 1.5 小时** | |

---

## 三、改 main.cpp（A 同学负责）

main.cpp 是程序入口，管路由（浏览器访问哪个网址执行哪段代码）。

### 要做的事情

**1. 删掉不需要的 #include**
删掉 `#include "enhance.h"` 和 `#include "extra.h"`

**2. 删掉不需要的路由**
现在有约 60 条路由，删掉跟 enhance、extra、家长相关的，剩约 25 条。比如：
```
删掉：/api/goals, /api/points, /api/checkin, /api/parent/...
保留：/api/login, /api/students, /api/grades, /api/stats/...
```

**3. 看不懂的写法改成看得懂的**

现在 main.cpp 里有很多 lambda 表达式（`[](){}` 这种），大一看着懵。改成普通函数：

```cpp
// ===== 现在（看不懂）=====
svr.Get("/api/students", [](const Request& req, Response& res) {
    auto students = student::listAll();
    json j = students;
    res.set_content(j.dump(), "application/json");
});

// ===== 改成（看得懂）=====
// 先写一个普通函数
void 处理学生列表请求(const Request& 请求, Response& 响应) {
    json 学生数据 = student::listAll();
    响应.set_content(学生数据.dump(), "application/json");
}

// 再注册路由
svr.Get("/api/students", 处理学生列表请求);
```

**4. 删除 seedData() 里面的额外代码**

seedData() 是初始化测试数据的函数。删掉里面生成 error、goal、point、checkin 数据的代码段（约 50 行）。

---

## 四、不改的东西（拿来就用）

这些东西不用理解它怎么实现的，当成工具直接用就行：

| 东西 | 文件 | 怎么用 |
|------|------|--------|
| HTTP 服务器 | `lib/httplib.h` | 复制粘贴就行，一个头文件 |
| JSON 解析 | `lib/json.hpp` | 用 `json j; j["name"] = "张三";` 存数据 |
| CMake | `CMakeLists.txt` | 不用管，`cmake .. && make` 就能编译 |

---

## 五、改 CMakeLists.txt

只需要改一行，把 enhance 和 extra 从编译列表里删掉：

```cmake
# 原来（9个文件）
set(SOURCES
    src/main.cpp
    src/util.cpp
    src/auth.cpp
    src/student.cpp
    src/exam.cpp
    src/grade.cpp
    src/stats.cpp
    src/extra.cpp      ← 删掉这行
    src/enhance.cpp    ← 删掉这行
)

# 改后（7个文件）
set(SOURCES
    src/main.cpp
    src/util.cpp
    src/auth.cpp
    src/student.cpp
    src/exam.cpp
    src/grade.cpp
    src/stats.cpp
)
```

---

## 六、干的顺序

1. **A 同学** 删掉 `enhance.cpp/h`、`extra.cpp/h`，改 CMakeLists.txt
2. **A 同学** 改 main.cpp（删路由、lambda 转函数、删 seedData）
3. **B/C/D 同学** 检查各自的模块（auth/student、exam/grade、stats），看有没有引用被删掉的东西，有的话去掉
4. **E 同学** 写 3 个 HTML 页面 + 1 个 CSS 文件
5. 编译跑一下，看能不能正常启动
6. 浏览器打开测试：登录 → 录入成绩 → 查看排名

---

## 七、你需要学会的东西对照

| 课程要求 | 咱们的做法 | 谁做 | 难不难 |
|---------|-----------|------|-------|
| 用 class 设计 | User → Student/Admin 继承 | B 同学 | ✅ 学过 |
| 文件存数据 | util.cpp 里封装了 fstream 读 JSON | A 同学 | ✅ 学过 |
| 排序算法 | stats.cpp 里按总分/单科排序 | D 同学 | ✅ 学过 |
| 两个角色登录 | auth.cpp 验证密码 | B 同学 | ✅ 学过 |
| 增删改查 | 每个模块都有 CRUD 函数 | B/C 同学 | ✅ 学过 |
| 加分：Web 前端 | 3 个 HTML 页面 | E 同学 | ⭐ 学 2 小时就行 |
| 加分：跨平台 | CMake 配好了 | 不需要动 | ⭐ 现成的 |
