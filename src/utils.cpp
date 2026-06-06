/*
 * utils.cpp — utils 模块的实现文件
 *
 * 本文件实现了 utils.h 中声明的所有工具函数。
 * 用到了以下 C++ 标准库：
 *   <fstream>  — 文件读写（ifstream / ofstream）
 *   <sstream>  — 字符串流（stringstream 用于数字→字符串转换）
 *   <ctime>    — 时间函数（time / localtime_s / strftime）
 *   <algorithm>— 算法（std::transform 用于字符串大小写转换）
 *   <map>      — 键值对映射表（Token → 用户ID 的存储）
 *   <cstdlib>  — srand / rand 随机数
 *   <direct.h> — Windows _mkdir 函数
 *   <io.h>     — Windows _access 函数
 *
 * 阅读建议：
 *   大一同学可以按函数逐个阅读，先看懂"输入→处理→输出"的大框架，
 *   再细看每行代码的具体写法。遇到不认识的函数，查 C++ Reference 是最佳选择。
 */
#include "utils.h"
#include <fstream>
#include <sstream>
#include <ctime>
#include <algorithm>
#include <map>
#include <cstdlib>

// ============================================================
// 【跨平台文件操作】Windows vs macOS/Linux
// ============================================================
// 不同操作系统对文件和目录的操作函数不同：
//   - 检查文件是否存在：Windows 用 _access, POSIX 用 access
//   - 创建目录：Windows 用 _mkdir (仅目录名), POSIX 用 mkdir (需加权限)
//   - 获取本地时间：Windows 用 localtime_s, POSIX 用 localtime_r
// ============================================================
#ifdef _WIN32
    #include <direct.h>    // _mkdir
    #include <io.h>        // _access
#else
    #include <sys/stat.h>  // mkdir / access
    #include <unistd.h>    // access
#endif

namespace utils {

    // =================================================================
    // 一、目录操作
    //   负责创建数据目录和检测文件是否存在。
    //   这两个函数通常在其他函数写文件之前被调用。
    // =================================================================

    /*
     * ensureDataDir()
     *   检查数据目录 data/ 是否存在，如果不存在就创建。
     *   无参数，无返回值。
     *
     * 核心逻辑：
     *   _access(path, 0) — Windows API，检查文件/目录是否可访问
     *     返回值 0 = 存在，非 0 = 不存在
     *   _mkdir(path)      — Windows API，创建目录
     */
    void ensureDataDir() {
        // Windows：_access 检查目录是否存在，_mkdir 创建目录
        // macOS/Linux：access(F_OK) 检查存在，mkdir(path, 0755) 创建（0755=用户可读写执行，组和其他只读执行）
#ifdef _WIN32
        if (_access(DATA_DIR.c_str(), 0) != 0) {
            _mkdir(DATA_DIR.c_str());
#else
        if (access(DATA_DIR.c_str(), F_OK) != 0) {
            mkdir(DATA_DIR.c_str(), 0755);
#endif
        }
    }

    /*
     * fileExists(path)
     *   检测指定路径的文件是否存在。
     *   参数：path — 文件路径字符串
     *   返回：true 存在 / false 不存在
     */
    bool fileExists(const std::string& path) {
        // Windows：_access(path, 0) 检查文件是否存在
        // macOS/Linux：access(path, F_OK) 检查文件是否存在
#ifdef _WIN32
        return _access(path.c_str(), 0) == 0;
#else
        return access(path.c_str(), F_OK) == 0;
#endif
    }

    // =================================================================
    // 二、密码哈希 — DJB2 算法
    //   DJB2 由 Daniel J. Bernstein 设计，是经典的字符串哈希算法。
    //   特点：实现简单、分布均匀、冲突率低。
    //   注意：哈希 ≠ 加密，哈希结果不可逆推回原文。
    // =================================================================

    /*
     * hashPassword(str)
     *   输入：明文字符串 str
     *   输出：十六进制哈希值字符串
     *
     * DJB2 核心公式（循环中对每个字符执行一次）：
     *   hash = hash * 33 + c      （等价于 hash = (hash << 5) + hash + c）
     *   初始值 hash = 5381
     *
     * 为什么初始值是 5381？这是 DJB 通过实验选出的"魔术数字"，
     * 能让哈希值分布更均匀，减少冲突概率。
     */
    std::string hashPassword(const std::string& str) {
        size_t hash = 5381;                    // DJB2 的初始魔数
        // 遍历字符串中的每个字符
        for (size_t i = 0; i < str.size(); i++) {
            char c = str[i];
            // (hash << 5) + hash 等价于 hash * 33，位运算更快
            // (unsigned char)c 确保字符转为无符号数，避免负数干扰
            hash = ((hash << 5) + hash) + (unsigned char)c;
        }
        // 将哈希值转为十六进制字符串
        std::stringstream ss;
        ss << std::hex << hash;                // std::hex 使输出为十六进制格式
        return ss.str();
    }

    // =================================================================
    // 三、Token 管理（内存存储）
    //   Token 就像是"临时身份证"。用户登录后，系统生成一个随机串（token）
    //   交给用户，后续请求只需携带 token 即可证明身份。
    //
    //   存储方式：使用两个 static map（静态变量，程序运行期间常驻内存）
    //     tokenToUserId: token → userId 映射（验证 token 是否有效）
    //     userRoles:     userId → role 映射（查询用户角色）
    //
    //   注意：数据存在内存中，程序重启后所有 token 失效，用户需重新登录。
    //   这是最简单的 token 管理方案，适合学习，不适合生产环境。
    // =================================================================

    // token → 用户ID 的映射表（登录时写入，后续请求通过它查身份）
    static std::map<std::string, std::string> tokenToUserId;
    // 用户ID → 角色的映射表（登录时写入，用于权限判断）
    static std::map<std::string, std::string> userRoles;

    /*
     * getUserIdByToken(token)
     *   从 tokenToUserId 映射表中查询 token 对应的用户 ID。
     *   参数：token — 登录令牌
     *   返回：用户 ID 字符串；未找到返回 "-1"
     *
     * map::count(key) — 检查 key 是否存在（返回 0 或 1）
     * map::operator[] — 访问 key 对应的 value
     */
    std::string getUserIdByToken(const std::string& token) {
        // count(token) > 0 表示映射表中存在这个 token
        if (tokenToUserId.count(token)) return tokenToUserId[token];
        return "-1";                           // 不存在则返回 "-1" 表示无效
    }

    /*
     * storeToken(token, userId)
     *   注册 token → userId 的映射，相当于"把 token 和用户绑定"。
     *   参数：token — 令牌, userId — 用户 ID
     *   无返回值
     */
    void storeToken(const std::string& token, const std::string& userId) {
        tokenToUserId[token] = userId;         // map 的 [] 运算符：存在则修改，不存在则插入
    }

    /*
     * isTokenValid(token)
     *   判断 token 是否有效。两个条件：
     *     1. token 存在于映射表中
     *     2. 对应的 userId 不是 "-1"（"-1" 是我们的"无效"标记）
     */
    bool isTokenValid(const std::string& token) {
        return tokenToUserId.count(token) > 0 && tokenToUserId[token] != "-1";
    }

    /*
     * getUserRoleByToken(token)
     *   两步查询：token → userId → role
     *   参数：token — 令牌
     *   返回：角色字符串；查不到返回空串 ""
     */
    std::string getUserRoleByToken(const std::string& token) {
        std::string uid = getUserIdByToken(token); // 第一步：token 转 userId
        if (uid != "-1" && userRoles.count(uid)) return userRoles[uid]; // 第二步：userId 转 role
        return "";                               // 任一环节失败则返回空串
    }

    /*
     * setUserRole(userId, role)
     *   设置用户角色，登录成功后调用此函数把角色信息存入内存。
     */
    void setUserRole(const std::string& userId, const std::string& role) {
        userRoles[userId] = role;
    }

    // =================================================================
    // 四、Token 生成
    //   使用 C 标准库的 rand() 函数生成随机十六进制字符串。
    //   这不是密码学安全的随机数，仅适用于学习和非生产环境。
    // =================================================================

    /*
     * generateToken()
     *   生成一个 16 位随机十六进制字符串作为登录令牌。
     *   无参数
     *   返回：16 个十六进制字符，如 "a3f0c81b5d2e947f"
     *
     * 核心逻辑：
     *   1. 首次调用时用当前时间戳设置随机种子（保证每次启动种子不同）
     *   2. 循环 16 次，每次 rand() % 16 从 hex 表中取一个字符
     *
     * static bool seeded — 确保 srand 只执行一次。
     *   如果每次生成 token 都重新 srand，短时间内产生的 token
     *   可能完全相同（因为 time(NULL) 到秒级，同一秒内值相同）。
     */
    std::string generateToken() {
        static bool seeded = false;            // static 变量只初始化一次，记录是否已播种
        if (!seeded) {
            // 用当前时间戳（秒级）作为随机种子
            srand((unsigned int)time(NULL));
            seeded = true;                     // 标记为已播种，下次不再执行
        }
        const char hex[] = "0123456789abcdef"; // 十六进制字符表
        std::string token;
        // 循环 16 次，每次随机选一个十六进制字符拼接到 token 末尾
        for (int i = 0; i < 16; i++) {
            token += hex[rand() % 16];         // rand()%16 得到 0~15 的随机索引
        }
        return token;
    }

    // =================================================================
    // 五、时间获取
    //   将系统当前时间转为可读的 "年-月-日 时:分:秒" 格式。
    // =================================================================

    /*
     * getCurrentTime()
     *   获取当前系统时间，返回格式化字符串。
     *   无参数
     *   返回：如 "2025-06-03 14:30:00"
     *
     * 调用链：
     *   time(NULL)        → 获取从1970-01-01至今的秒数（时间戳）
     *   localtime_s()     → 将时间戳转为本地时间的 tm 结构体
     *   strftime()        → 将 tm 结构体按格式串转为字符串
     *
     * localtime_s 是 localtime 的安全版本（Windows/MSVC 推荐）。
     */
    std::string getCurrentTime() {
        time_t t = time(NULL);
        struct tm timeinfo;
        // Windows：localtime_s（安全版本，参数顺序是(&tm, &time_t)）
        // macOS/Linux：localtime_r（POSIX 版本，参数顺序是(&time_t, &tm)）
#ifdef _WIN32
        localtime_s(&timeinfo, &t);
#else
        localtime_r(&t, &timeinfo);
#endif
        char buf[32];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
        return std::string(buf);
    }

    // =================================================================
    // 六、字符串工具函数
    //   这些是任何 C++ 项目都可能用到的基础字符串操作。
    //   掌握它们对理解 <string> 和 <algorithm> 很有帮助。
    // =================================================================

    /*
     * toLower(s)
     *   将字符串中的大写字母转为小写（其他字符不变）。
     *   参数：s — 原始字符串
     *   返回：全部小写的字符串
     *
     * std::transform(起始, 结束, 写入起始, 操作函数)
     *   — 对范围内每个元素执行操作函数，结果写回原位置
     * ::tolower — 全局函数，将单个字符转为小写
     */
    std::string toLower(const std::string& s) {
        std::string r = s;                     // 拷贝一份，不修改原字符串
        // transform 对 r 中每个字符调用 ::tolower，结果覆盖写回 r
        std::transform(r.begin(), r.end(), r.begin(), ::tolower);
        return r;
    }

    /*
     * trim(s)
     *   去除字符串首尾的空白字符（空格、\t、\r、\n）。
     *   参数：s — 原始字符串
     *   返回：去掉首尾空白后的子串
     *
     * 算法：双指针法
     *   start 指针：从左向右移动，跳过空白字符
     *   end 指针：  从右向左移动，跳过空白字符
     *   最终返回 s[start ... end-1] 这段子串
     */
    std::string trim(const std::string& s) {
        size_t start = 0;
        // 从左跳过空白：只要当前字符是空白且未越界，start 就右移
        while (start < s.size() && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r' || s[start] == '\n')) start++;
        size_t end = s.size();
        // 从右跳过空白：只要前一个字符是空白且 end > start，end 就左移
        while (end > start && (s[end-1] == ' ' || s[end-1] == '\t' || s[end-1] == '\r' || s[end-1] == '\n')) end--;
        // substr(start, length): 从 start 位置开始取 length 个字符
        return s.substr(start, end - start);
    }

    /*
     * split(s, delim)
     *   按分隔符将字符串切成多个子串。
     *   参数：s — 原始字符串, delim — 分隔字符
     *   返回：vector<string> 存放切好的子串
     *
     * 算法：遍历每个字符，遇到分隔符就把当前积累的串存入结果并清空，
     *        否则把字符追加到当前串末尾。
     *        遍历结束后要把最后一个积累的串也存入结果。
     *
     * 示例：split("张三,李四,王五", ',') → {"张三", "李四", "王五"}
     */
    std::vector<std::string> split(const std::string& s, char delim) {
        std::vector<std::string> result;       // 存放切分结果
        std::string current;                   // 当前正在积累的子串
        for (size_t i = 0; i < s.size(); i++) {
            if (s[i] == delim) {
                // 遇到分隔符：当前积累的子串结束，存入结果
                result.push_back(current);
                current.clear();               // 清空 current，准备积累下一个子串
            } else {
                // 非分隔符：追加到当前子串
                current += s[i];
            }
        }
        // 循环结束后 current 中还有最后一个子串（末尾没有分隔符），也要存进去
        result.push_back(current);
        return result;
    }

    /*
     * join(parts, delim)
     *   将字符串列表用分隔符拼接成一个字符串（split 的逆操作）。
     *   参数：parts — 字符串列表, delim — 分隔符
     *   返回：拼接后的字符串
     *
     * 逻辑：遍历每个元素，如果不是第一个就在前面加分隔符。
     */
    std::string join(const std::vector<std::string>& parts, const std::string& delim) {
        std::string result;
        for (size_t i = 0; i < parts.size(); i++) {
            if (i > 0) result += delim;        // 不是第一个元素，先拼分隔符
            result += parts[i];                // 再拼当前元素
        }
        return result;
    }

    /*
     * startsWith(s, prefix)
     *   判断 s 是否以 prefix 开头。
     *   参数：s — 字符串, prefix — 前缀
     *   返回：true / false
     *
     * 逻辑：先检查 s 的长度 ≥ prefix 的长度（否则不可能以 prefix 开头），
     *        再比较 s 的前几个字符是否与 prefix 完全相同。
     */
    bool startsWith(const std::string& s, const std::string& prefix) {
        // 长度不够 prefix 长 → 一定不是其前缀
        // 取 s 的前 prefix.size() 个字符，与 prefix 比较
        return s.size() >= prefix.size() && s.substr(0, prefix.size()) == prefix;
    }

    // =================================================================
    // 七、数学工具
    //   成绩管理中的"分数美化"和"等级转换"。
    // =================================================================

    /*
     * round2(val)
     *   四舍五入保留两位小数。
     *   参数：val — 原始值（如 85.678）
     *   返回：保留两位小数的值（如 85.68）
     *
     * 算法思路（因为 C++ 没有内置"保留 n 位小数"的四舍五入函数）：
     *   1. val × 100        → 将小数左移两位变成整数部分（如 85.678 × 100 = 8567.8）
     *   2. + 0.5 然后取整    → long long 强转会截断小数，+0.5 实现四舍五入
     *   3. ÷ 100.0          → 再还原为原数量级
     *
     * 注意：long long 的强转是向下截断（truncate），不是四舍五入。
     *       所以要先 +0.5，让 >=0.5 的部分进位再截断。
     *
     * 示例：round2(85.678)
     *   85.678 × 100 = 8567.8
     *   8567.8 + 0.5 = 8568.3
     *   (long long)8568.3 = 8568
     *   8568 / 100.0 = 85.68 ✓
     */
    double round2(double val) {
        val = val * 100;                       // 放大 100 倍
        long long rounded = (long long)(val + 0.5); // +0.5 后截断实现四舍五入
        return (double)rounded / 100.0;        // 缩回原数量级
    }

    /*
     * gradeToLetter(percent)
     *   百分制 → 字母等级（美国大学常见等级制度）。
     *   参数：percent — 百分制分数
     *   返回：A / B / C / D / F
     *
     * 注意：if-else 的顺序很重要！
     *       因为是从高到低判断，满足 90 就不会走到 80 分支。
     *       如果反过来写（先判断 ≥60），那 95 分也会返回 "D"。
     */
    std::string gradeToLetter(double percent) {
        if (percent >= 90) return "A";         // 90-100 → A（优秀）
        if (percent >= 80) return "B";         // 80-89  → B（良好）
        if (percent >= 70) return "C";         // 70-79  → C（中等）
        if (percent >= 60) return "D";         // 60-69  → D（及格）
        return "F";                            // 0-59   → F（不及格）
    }

    /*
     * gradeToGPA(percent)
     *   百分制 → GPA（Grade Point Average，平均学分绩点）。
     *   参数：percent — 百分制分数
     *   返回：对应 GPA 值（0.0 ~ 4.0）
     *
     * GPA 等级划分比字母等级更细，中间有 3.7、3.3、2.7、2.3、1.5 等中间档。
     * 这是常见的 4.0 制 GPA 换算标准，不同学校可能略有差异。
     */
    double gradeToGPA(double percent) {
        if (percent >= 90) return 4.0;
        if (percent >= 85) return 3.7;
        if (percent >= 82) return 3.3;
        if (percent >= 78) return 3.0;
        if (percent >= 75) return 2.7;
        if (percent >= 72) return 2.3;
        if (percent >= 68) return 2.0;
        if (percent >= 64) return 1.5;
        if (percent >= 60) return 1.0;
        return 0.0;                            // 60 以下 → 0.0
    }

    // =================================================================
    // 八、JSON 响应构建
    //   本系统未使用第三方 JSON 库（如 nlohmann/json），
    //   而是手动拼接字符串来构造 JSON。这样做的好处是：
    //     - 不需要额外依赖
    //     - 代码逻辑一目了然，适合学习
    //   缺点是：
    //     - 代码冗余，每个 JSON 都要手动拼
    //     - 嵌套复杂时容易出错
    // =================================================================

    /*
     * jsonEscape(s)
     *   转义 JSON 字符串中的特殊字符。
     *   为什么需要转义？
     *     JSON 中字符串值用双引号包裹。如果字符串本身包含双引号，
     *     就会提前结束字符串，导致 JSON 格式错误。
     *     例如：{"msg":"He said "hello""} ← 双引号冲突，JSON 解析失败
     *     转义后：{"msg":"He said \"hello\""}  ← 正确
     *
     *   参数：s — 需要转义的原始字符串
     *   返回：转义后的字符串
     */
    std::string jsonEscape(const std::string& s) {
        std::string r;
        for (size_t i = 0; i < s.size(); i++) {
            if (s[i] == '"') r += "\\\"";      // 双引号转义
            else if (s[i] == '\\') r += "\\\\"; // 反斜杠转义
            else if (s[i] == '\r') r += "\\r";   // 回车转义
            else if (s[i] == '\n') r += "\\n";   // 换行转义
            else if (s[i] == '\t') r += "\\t";   // 制表转义
            else r += s[i];                       // 普通字符不变
        }
        return r;
    }

    /*
     * jsonResponse(success, message, data)
     *   构建标准的 JSON 响应。
     *   参数：
     *     success — 操作是否成功
     *     message — 提示消息（会自动转义）
     *     data    — JSON 数据部分（调用者已拼好的片段）
     *   返回：
     *     完整的 JSON 字符串
     *
     * 注意：data 参数不经过 jsonEscape 转义（因为它已经是合法的 JSON 片段）。
     *       只有 message 需要转义（因为它是普通文本）。
     */
    std::string jsonResponse(bool success, const std::string& message, const std::string& data) {
        std::string r = "{\"success\":";
        r += success ? "true" : "false";       // 根据 success 布尔值拼接 true/false
        r += ",\"message\":\"" + jsonEscape(message) + "\""; // message 需要转义
        r += ",\"data\":" + data;              // data 是已拼好的 JSON，直接拼接
        r += "}";
        return r;
    }

    /*
     * errorResponse(message)
     *   快捷构建失败响应：success=false，data 为空对象。
     *   等价于 jsonResponse(false, message, "{}")
     */
    std::string errorResponse(const std::string& message) {
        return jsonResponse(false, message, "{}");
    }

    // =================================================================
    // 九、URL 参数解析
    //   从类似 "name=张三&age=20" 的查询字符串中提取指定参数的值。
    //   这在处理 HTTP GET 请求的 URL 参数时非常常用。
    // =================================================================

    /*
     * getQueryParam(queryString, key)
     *   解析 URL 查询参数。
     *   参数：
     *     queryString — URL 查询部分，例如 "id=1001&name=张三"
     *     key         — 要提取的参数名
     *   返回：参数值；参数不存在返回 ""
     *
     * 核心逻辑（以 queryString="id=1001&name=张三", key="id" 为例）：
     *   1. 拼搜索串 "id="
     *   2. 在 "id=1001&name=张三" 中找 "id=" → 位置 0
     *   3. 值的起始位置 = 0 + 3("id=" 的长度) = 3，即字符 '1' 的位置
     *   4. 从位置 3 开始找下一个 "&" → 位置 7
     *   5. 取 [3, 7) 子串 → "1001" ✓
     *
     * string::npos 是 string 的静态常量，值为 size_t 的最大值，
     * 表示"未找到"。find() 找不到目标时返回 npos。
     */
    std::string getQueryParam(const std::string& queryString, const std::string& key) {
        std::string searchKey = key + "=";     // 拼出搜索串，如 "id="
        // 第一步：查找 "key=" 的位置
        size_t pos = queryString.find(searchKey);
        if (pos == std::string::npos) return ""; // 没找到这个 key，返回空串
        // 第二步：跳过 "key="，定位到值的起始位置
        pos += searchKey.size();
        // 第三步：在 pos 之后找下一个 "&"（参数分隔符），作为值的结束位置
        size_t end = queryString.find("&", pos);
        // 如果找不到 "&"，说明这是最后一个参数，值到字符串末尾
        if (end == std::string::npos) end = queryString.size();
        // 第四步：截取 [pos, end) 的子串作为参数值返回
        return queryString.substr(pos, end - pos);
    }

    // =================================================================
    // 十、日志系统
    //   将用户操作记录到文件中，支持分页查询。
    //   日志格式：每行一条记录，字段用 "|" 分隔。
    // =================================================================

    /*
     * logAction(userId, action, detail)
     *   追加一条操作日志到文件 data/logs.txt。
     *   参数：
     *     userId — 操作用户
     *     action — 操作类型
     *     detail — 操作详情
     *   无返回值
     *
     * 存储格式（每行）：
     *   用户ID|操作类型|详情|时间戳
     *   "|" 作为分隔符（选择 | 而非 , 是因为日志内容可能包含逗号）
     *
     * ios::app — 追加模式：写入内容追加到文件末尾，不会覆盖已有内容。
     * ofstream 在函数结束时自动析构，析构时会自动关闭文件。
     */
    void logAction(const std::string& userId, const std::string& action, const std::string& detail) {
        ensureDataDir();                         // 先确保数据目录存在
        std::ofstream f(DATA_DIR + "logs.txt", std::ios::app); // 以追加模式打开
        // 写入一行，格式：userId|action|detail|时间
        f << userId << "|" << action << "|" << detail << "|" << getCurrentTime() << "\n";
        // f 在离开作用域时自动关闭（RAII — 资源获取即初始化）
    }

    /*
     * getLogs(page, size, userFilter)
     *   分页查询日志，返回 JSON 格式。
     *   参数：
     *     page       — 页码（从 1 开始，第 1 页、第 2 页...）
     *     size       — 每页的条数
     *     userFilter — 按用户筛选（只显示匹配的用户的日志），空串 = 不筛选
     *   返回：JSON 字符串
     *
     * 处理流程：
     *   ① 打开文件，逐行读取
     *   ② 如果设置了 userFilter，跳过不包含该关键词的行
     *   ③ 将所有行存入 vector，然后反转（让最新日志排在最前面）
     *   ④ 按 page 和 size 计算切片范围 [start, end)
     *   ⑤ 逐条拼接为 JSON 数组
     *
     * 分页计算公式：
     *   start = (page - 1) × size    （当前页第一条记录的索引）
     *   end   = start + size         （当前页最后一条记录的下一个位置）
     *   如果 end > total，说明是最后一页不够 size 条，end = total
     */
    std::string getLogs(int page, int size, const std::string& userFilter) {
        std::string path = DATA_DIR + "logs.txt";
        std::ifstream f(path);
        // 如果文件打不开（比如还没创建），返回空数据
        if (!f.is_open())
            return "{\"total\":0,\"page\":" + std::to_string(page) + ",\"size\":" + std::to_string(size) + ",\"data\":[]}";

        // ① 读取所有匹配的日志行
        std::vector<std::string> allLines;
        std::string line;
        while (std::getline(f, line)) {        // getline 每次读一行（遇 \n 停止）
            if (line.empty()) continue;        // 跳过空行
            // 如果设置了筛选条件，检查当前行是否包含筛选关键字
            if (!userFilter.empty() && line.find(userFilter) == std::string::npos) continue;
            allLines.push_back(line);
        }

        // ② 反转顺序：最新的日志在最前面（数组开头）
        std::vector<std::string> reversed;
        for (int i = (int)allLines.size() - 1; i >= 0; i--)
            reversed.push_back(allLines[i]);

        // ③ 计算分页范围
        int total = (int)reversed.size();
        int start = (page - 1) * size;         // 起始索引
        int end = start + size;                // 结束索引（不含）
        if (end > total) end = total;          // 不超过总条数

        // ④ 拼接 JSON：先写元数据
        std::string data = "{\"total\":" + std::to_string(total) + ",\"page\":" + std::to_string(page) +
                           ",\"size\":" + std::to_string(size) + ",\"data\":[";
        // ⑤ 逐条拼接日志为 JSON 对象
        for (int i = start; i < end; i++) {
            if (i > start) data += ",";        // 不是第一个对象，加逗号分隔
            // 用 | 切分每条日志：userId|action|detail|time
            std::vector<std::string> parts = split(reversed[i], '|');
            data += "{";
            // parts[0] = 用户ID, parts[1] = 操作类型, parts[2] = 详情, parts[3] = 时间
            // 使用三元运算符防止 parts 不足 4 项时越界
            data += "\"user\":\"" + jsonEscape(parts.size() > 0 ? parts[0] : "") + "\",";
            data += "\"action\":\"" + jsonEscape(parts.size() > 1 ? parts[1] : "") + "\",";
            data += "\"detail\":\"" + jsonEscape(parts.size() > 2 ? parts[2] : "") + "\",";
            data += "\"time\":\"" + jsonEscape(parts.size() > 3 ? parts[3] : "") + "\"";
            data += "}";
        }
        data += "]}";
        return data;
    }

}
