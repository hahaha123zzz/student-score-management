#pragma once

#include <string>
#include <vector>

/*
 * utils 命名空间：提供本系统所需的各种工具函数，包括：
 *   目录操作、密码哈希、Token 管理、日志记录、
 *   时间获取、字符串处理、数学计算、JSON 拼接、URL 解析
 *
 * 本模块中所有函数都是独立的"工具函数"——给定输入，返回输出，
 * 不依赖外部的全局状态（除 Token 管理的内存映射外）。
 * 适合大一同学学习"如何从零实现一个工具库"。
 */
namespace utils {

    /*
     * DATA_DIR: 数据文件存放目录，所有持久化文件（如日志、用户数据）
     * 都保存在这个目录下。使用 const 保证其值不会被意外修改。
     */
    const std::string DATA_DIR = "data/";

    // ======================= 目录操作 =======================

    /*
     * ensureDataDir()
     *   确保 DATA_DIR 目录存在，如果不存在则创建它。
     *   参数：无
     *   返回：无
     *   典型用法：在写文件之前调用，避免因目录不存在而写入失败。
     */
    void ensureDataDir();

    /*
     * fileExists(path)
     *   检查指定路径的文件或目录是否存在。
     *   参数：
     *     path — 文件/目录的路径（std::string 类型，引用传递避免拷贝）
     *   返回值：
     *     true  — 存在
     *     false — 不存在
     *   底层：使用 Windows 的 _access 函数做可访问性检测。
     */
    bool fileExists(const std::string& path);

    // ======================= 密码哈希 =======================

    /*
     * hashPassword(str)
     *   使用 DJB2 哈希算法将明文密码转换为十六进制哈希字符串。
     *   （DJB2 是一种简单、经典的字符串哈希算法，常用于练习。）
     *   参数：
     *     str — 待哈希的明文字符串
     *   返回值：
     *     std::string — 哈希结果的十六进制表示，如 "7c9a2e"
     *   注意：这不是加密，也不可逆，仅用于演示"不存明文密码"的思想。
     */
    std::string hashPassword(const std::string& str);

    // ======================= Token 管理 =======================

    /*
     * generateToken()
     *   生成一个 16 位十六进制随机字符串作为用户登录令牌。
     *   参数：无
     *   返回值：
     *     std::string — 16 个十六进制字符组成的 token，如 "a3f0c81b5d2e947f"
     *   实现：首次调用时用 srand(time(NULL)) 设置随机种子（仅设一次），
     *         之后每次调用通过 rand() % 16 从 hex 表随机取字符。
     */
    std::string generateToken();

    /*
     * getUserIdByToken(token)
     *   根据 token 查找对应的用户 ID。
     *   参数：
     *     token — 待查询的登录令牌
     *   返回值：
     *     对应的用户 ID 字符串；若 token 不存在，返回 "-1"
     *   数据来源：内存中的 map<string,string> tokenToUserId
     */
    std::string getUserIdByToken(const std::string& token);

    /*
     * storeToken(token, userId)
     *   将 token -> userId 的映射存入内存表，完成"登录登记"。
     *   参数：
     *     token  — 登录令牌
     *     userId — 用户 ID
     *   返回：无
     *   注意：数据仅存在内存中，程序重启后 token 全部失效。
     */
    void storeToken(const std::string& token, const std::string& userId);

    /*
     * isTokenValid(token)
     *   判断一个 token 是否有效（即是否存在且对应有效用户）。
     *   参数：
     *     token — 待校验的令牌
     *   返回值：
     *     true  — 有效
     *     false — 无效或不存在
     */
    bool isTokenValid(const std::string& token);

    /*
     * getUserRoleByToken(token)
     *   通过 token 获取当前用户的角色（如 "admin" / "student" / "teacher"）。
     *   参数：
     *     token — 登录令牌
     *   返回值：
     *     角色字符串；若 token 无效或角色未设置，返回空串 ""
     *   核心步骤：
     *     1. token → userId（调用 getUserIdByToken）
     *     2. userId → role（从 userRoles 映射表查询）
     */
    std::string getUserRoleByToken(const std::string& token);

    /*
     * setUserRole(userId, role)
     *   设置用户的角色（如登录时从文件读取角色信息后调用）。
     *   参数：
     *     userId — 用户 ID
     *     role   — 角色名称（"admin" / "student" / "teacher"）
     *   返回：无
     */
    void setUserRole(const std::string& userId, const std::string& role);

    // ======================= 日志 =======================

    /*
     * logAction(userId, action, detail)
     *   向日志文件追加一条操作记录。
     *   参数：
     *     userId — 执行操作的用户 ID
     *     action — 操作类型描述（如 "login", "add_score"）
     *     detail — 操作详情（如 "添加了张三的数学成绩"）
     *   返回：无
     *   存储格式（每行）：
     *     userId|action|detail|时间
     *   文件路径：data/logs.txt，以追加模式打开。
     */
    void logAction(const std::string& userId, const std::string& action, const std::string& detail);

    /*
     * getLogs(page, size, userFilter)
     *   分页查询日志，返回 JSON 格式的结果，最新的日志排在前面。
     *   参数：
     *     page       — 页码（从 1 开始）
     *     size       — 每页条数
     *     userFilter — 用户筛选关键词（空串表示不筛选）
     *   返回值：
     *     JSON 字符串，格式：
     *     {"total":总条数, "page":当前页, "size":每页条数,
     *      "data":[{"user":"...","action":"...","detail":"...","time":"..."}, ...]}
     *   核心逻辑：
     *     1. 读取全部日志行
     *     2. 按 userFilter 筛选（简单字符串包含匹配）
     *     3. 反转行顺序 → 最新的在前
     *     4. 按 page/size 切片返回
     */
    std::string getLogs(int page, int size, const std::string& userFilter);

    // ======================= 时间 =======================

    /*
     * getCurrentTime()
     *   获取当前系统时间，格式化为 "年-月-日 时:分:秒"。
     *   参数：无
     *   返回值：
     *     std::string — 如 "2025-06-03 14:30:00"
     *   底层：time() 获取时间戳 → localtime_s() 转为本地时间 → strftime() 格式化
     */
    std::string getCurrentTime();

    // ======================= 字符串工具 =======================

    /*
     * toLower(s)
     *   将字符串中的大写字母全部转为小写。
     *   参数：
     *     s — 原始字符串
     *   返回值：
     *     全部小写的新字符串
     *   实现：利用 <algorithm> 中的 std::transform + ::tolower
     */
    std::string toLower(const std::string& s);

    /*
     * trim(s)
     *   去除字符串首尾的空白字符（空格、制表符、回车、换行）。
     *   参数：
     *     s — 原始字符串
     *   返回值：
     *     去掉首尾空白后的字符串
     *   核心逻辑：
     *     start 指针从左向右跳过空白 →
     *     end 指针从右向左跳过空白 →
     *     返回 s.substr(start, end - start)
     */
    std::string trim(const std::string& s);

    /*
     * split(s, delim)
     *   按分隔符将字符串切分为多个子串。
     *   参数：
     *     s     — 原始字符串
     *     delim — 分隔字符（单个 char），如 ',' 或 '|'
     *   返回值：
     *     vector<string> — 切分后的子串列表
     *   示例：
     *     split("张三,李四,王五", ',') → {"张三", "李四", "王五"}
     */
    std::vector<std::string> split(const std::string& s, char delim);

    /*
     * join(parts, delim)
     *   将字符串数组用分隔符拼接成一个字符串（split 的逆操作）。
     *   参数：
     *     parts — 待拼接的字符串列表
     *     delim — 分隔符
     *   返回值：
     *     拼接后的字符串
     *   示例：
     *     join({"张三", "李四", "王五"}, ", ") → "张三, 李四, 王五"
     */
    std::string join(const std::vector<std::string>& parts, const std::string& delim);

    /*
     * startsWith(s, prefix)
     *   判断字符串 s 是否以 prefix 开头。
     *   参数：
     *     s      — 待判断的字符串
     *     prefix — 前缀
     *   返回值：
     *     true  — s 以 prefix 开头
     *     false — 不是
     *   实现：先比较长度确保 s 足够长，再取 s 的前缀子串与 prefix 比较
     */
    bool startsWith(const std::string& s, const std::string& prefix);

    // ======================= 数学工具 =======================

    /*
     * round2(val)
     *   四舍五入保留两位小数。
     *   参数：
     *     val — 原始浮点数（如 85.678）
     *   返回值：
     *     保留两位小数的结果（如 85.68）
     *   核心逻辑：
     *     val × 100 → 转 long long 取整（+0.5 实现四舍五入）→ ÷ 100.0
     */
    double round2(double val);

    /*
     * gradeToLetter(percent)
     *   将百分制分数转换为字母等级（A/B/C/D/F）。
     *   参数：
     *     percent — 百分制分数（0~100）
     *   返回值：
     *     "A"（≥90）, "B"（≥80）, "C"（≥70）, "D"（≥60）, "F"（<60）
     *   注意：if-else 链从上到下判断，先匹配高分档。
     */
    std::string gradeToLetter(double percent);

    /*
     * gradeToGPA(percent)
     *   将百分制分数转换为 GPA（Grade Point Average，绩点）。
     *   参数：
     *     percent — 百分制分数（0~100）
     *   返回值：
     *     double — 对应的绩点值（0.0 ~ 4.0）
     *   映射规则：
     *     ≥90→4.0, ≥85→3.7, ≥82→3.3, ≥78→3.0,
     *     ≥75→2.7, ≥72→2.3, ≥68→2.0, ≥64→1.5, ≥60→1.0, 否则 0.0
     *   与 gradeToLetter 的区别：GPA 映射更细分，有 3.7 / 3.3 / 2.7 / 2.3 等中间档。
     */
    double gradeToGPA(double percent);

    // ======================= JSON 响应构建 =======================

    /*
     * jsonResponse(success, message, data)
     *   手动拼接一个 JSON 响应字符串。系统未引入第三方 JSON 库，
     *   直接用字符串拼接构建 JSON，适合轻量场景。
     *   参数：
     *     success — 操作是否成功
     *     message — 提示信息（会被 jsonEscape 转义）
     *     data    — JSON 数据部分（调用者已拼好的字符串，如 "{}" 或 "[...]"）
     *   返回值：
     *     完整 JSON 字符串，格式：
     *     {"success":true/false, "message":"...", "data":<data>}
     */
    std::string jsonResponse(bool success, const std::string& message, const std::string& data);

    /*
     * errorResponse(message)
     *   构建一个失败响应 JSON（success = false, data = {}）。
     *   参数：
     *     message — 错误提示信息
     *   返回值：
     *     JSON 字符串
     *   本质：jsonResponse(false, message, "{}") 的快捷调用。
     */
    std::string errorResponse(const std::string& message);

    /*
     * jsonEscape(s)
     *   对字符串中的 JSON 特殊字符进行转义，防止拼接 JSON 时格式出错。
     *   参数：
     *     s — 原始字符串
     *   返回值：
     *     转义后的字符串
     *   转义规则：
     *     " → \"
     *     \ → \\
     *     回车 \r → \\r
     *     换行 \n → \\n
     *     制表 \t → \\t
     */
    std::string jsonEscape(const std::string& s);

    // ======================= URL 参数解析 =======================

    /*
     * getQueryParam(queryString, key)
     *   从 URL 查询字符串中提取指定参数的值。
     *   参数：
     *     queryString — URL 查询部分，如 "name=张三&age=20"
     *     key         — 参数名，如 "name"
     *   返回值：
     *     参数值字符串；若参数不存在返回空串 ""
     *   核心逻辑：
     *     在 queryString 中查找 "key=" → 定位值的起始位置 →
     *     查找下一个 "&" 作为值的结束位置 → 返回中间的子串
     */
    std::string getQueryParam(const std::string& queryString, const std::string& key);

}
