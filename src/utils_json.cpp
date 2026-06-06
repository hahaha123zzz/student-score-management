/*
 * utils_json.cpp — utils 模块的 JSON 和日志工具实现
 *
 * 本文件实现了 utils.h 中声明的 JSON 响应构建、URL 解析和日志函数。
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

    // token → 用户ID 的映射表（登录时写入，后续请求通过它查身份）
    static std::map<std::string, std::string> tokenToUserId;
    // 用户ID → 角色的映射表（登录时写入，用于权限判断）
    static std::map<std::string, std::string> userRoles;

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
