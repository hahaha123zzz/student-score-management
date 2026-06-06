#include "server.h"
#include <iostream>
#include <fstream>
#include <sstream>

// ============================================================
// 【跨平台网络编程】不同操作系统的 Socket API
// ============================================================
// Windows 使用 WinSock2，需要链接 ws2_32.lib
//   - WSAStartup() / WSACleanup()：初始化和清理
//   - closesocket()：关闭连接
//   - SOCKET 是无符号整数类型
//
// macOS / Linux 使用 POSIX（Berkeley）Socket
//   - 头文件：<sys/socket.h> <netinet/in.h> <arpa/inet.h> <unistd.h>
//   - 不需要初始化和清理
//   - 直接用 close() 关闭连接（因为 socket 也是文件描述符）
//   - SOCKET 就是 int 类型
//
// 核心 API（socket / bind / listen / accept / recv / send）在两种系统上
// 名称和用法完全相同，所以业务代码不用改，只需处理头文件和类型定义。
// ============================================================

#ifdef _WIN32
    // Windows：使用 WinSock2
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")   // 自动链接 WinSock 库
    #define CLOSE_SOCKET(s) closesocket(s)
    // SOCKET 类型在 <winsock2.h> 中已定义（无符号整数）
    // INVALID_SOCKET 和 SOCKET_ERROR 也已定义
#else
    // macOS / Linux：使用 POSIX Socket（Berkeley sockets）
    #include <sys/socket.h>      // socket / bind / listen / accept 等函数
    #include <netinet/in.h>      // sockaddr_in 结构体（IPv4 地址）
    #include <arpa/inet.h>       // htons / htonl 字节序转换函数
    #include <unistd.h>          // close 函数
    #define CLOSE_SOCKET(s) close(s)
    // POSIX 中 socket 返回 int，-1 表示错误
    typedef int SOCKET;
    #define INVALID_SOCKET (-1)
    #define SOCKET_ERROR   (-1)
#endif

namespace server {

    // 全局变量：路由处理函数指针 和 前端静态文件目录路径
    static RouteHandler g_handler = NULL;
    static std::string g_webDir;

    void setRouteHandler(RouteHandler handler) {
        g_handler = handler;
    }

    // ============================================================
    // 【HTTP 协议基础】
    // ============================================================
    // HTTP（超文本传输协议）是 Web 通信的标准格式。
    //
    // 一个 HTTP 请求的原始文本类似于：
    //   GET /api/users?id=1 HTTP/1.1\r\n          ← 请求行（方法 路径 HTTP版本）
    //   Host: localhost:8080\r\n                   ← 请求头（键值对）
    //   Authorization: Bearer token123\r\n
    //   \r\n                                        ← 空行（头与体的分隔）
    //   {"name":"张三"}                             ← 请求体（POST/PUT 才有）
    //
    // 本函数将原始 TCP 数据解析为 6 个字段：
    //   method:     请求方法（GET / POST / PUT / DELETE）
    //   path:       请求路径（如 /api/users）
    //   queryString:URL 查询字符串（? 后面的部分，如 id=1&name=张三）
    //   body:       请求体（POST/PUT 时提交的数据）
    //   token:      认证令牌（从 Authorization 头中提取）
    // ============================================================
    static void parseRequest(const std::string& raw,
                              std::string& method,
                              std::string& path,
                              std::string& queryString,
                              std::string& body,
                              std::string& token) {
        method = "";
        path = "/";
        queryString = "";
        body = "";
        token = "";

        // ----- 解析第一行（请求行）-----
        // 格式：GET /api/users?page=1 HTTP/1.1
        // 找到第一个 \r\n（行结束标记）
        size_t lineEnd = raw.find("\r\n");
        if (lineEnd == std::string::npos) return;
        std::string requestLine = raw.substr(0, lineEnd);

        // 第一个空格前是 method，最后一个空格后是 HTTP 版本
        // 中间部分（fullPath）是路径和查询字符串
        size_t space1 = requestLine.find(' ');
        size_t space2 = requestLine.rfind(' ');
        if (space1 == std::string::npos || space2 == std::string::npos) return;
        method = requestLine.substr(0, space1);
        std::string fullPath = requestLine.substr(space1 + 1, space2 - space1 - 1);

        // 分离路径和查询字符串
        // 例如 /api/users?id=1&name=张三 → path="/api/users", queryString="id=1&name=张三"
        size_t qpos = fullPath.find('?');
        if (qpos != std::string::npos) {
            path = fullPath.substr(0, qpos);
            queryString = fullPath.substr(qpos + 1);
        } else {
            path = fullPath;
        }

        // ----- 解析 Authorization 头，提取 Bearer Token -----
        // Authorization: Bearer eyJhbGciOi...（JSON Web Token）
        // 找到 "Authorization: Bearer " 后 22 个字符即为 token 起始位置
        size_t authPos = raw.find("Authorization: Bearer ");
        if (authPos != std::string::npos) {
            authPos += 22;   // "Authorization: Bearer " 的长度
            size_t authEnd = raw.find("\r\n", authPos);
            if (authEnd != std::string::npos)
                token = raw.substr(authPos, authEnd - authPos);
        }

        // ----- 解析请求体 -----
        // HTTP 协议中，头部和体之间用空行（\r\n\r\n）分隔
        // bodyStart 指向空行之后的内容
        size_t bodyStart = raw.find("\r\n\r\n");
        if (bodyStart != std::string::npos) {
            body = raw.substr(bodyStart + 4);   // +4 跳过 "\r\n\r\n" 四个字符
        }
    }

    // ============================================================
    // 【MIME 类型】根据文件扩展名返回对应的 Content-Type
    // ============================================================
    // MIME 类型告诉浏览器数据的格式，让它知道如何渲染
    //   .html  → text/html       （HTML 页面）
    //   .css   → text/css        （样式表）
    //   .js    → application/javascript （脚本）
    //   .json  → application/json（JSON 数据）
    // utf-8 声明字符编码，确保中文正常显示
    // ============================================================
    static std::string getMimeType(const std::string& path) {
        if (path.size() >= 5 && path.substr(path.size() - 5) == ".html") return "text/html; charset=utf-8";
        if (path.size() >= 4 && path.substr(path.size() - 4) == ".css") return "text/css; charset=utf-8";
        if (path.size() >= 3 && path.substr(path.size() - 3) == ".js") return "application/javascript; charset=utf-8";
        if (path.size() >= 4 && path.substr(path.size() - 4) == ".json") return "application/json; charset=utf-8";
        // 根路径 "/" 或无扩展名 → 默认按 HTML 处理
        if (path == "/" || path.find('.') == std::string::npos)
            return "text/html; charset=utf-8";
        return "text/plain; charset=utf-8";
    }

    // 读取前端静态文件（HTML / CSS / JS / JSON），从磁盘加载到内存并返回内容
    // 路径分隔符：Windows 用 \，macOS/Linux 用 /
    static std::string readStaticFile(const std::string& urlPath) {
#ifdef _WIN32
        const char SEP = '\\';
#else
        const char SEP = '/';
#endif
        if (urlPath == "/") {
            std::string fpath = g_webDir + SEP + "login.html";
            std::ifstream f(fpath, std::ios::binary);
            if (f.is_open()) {
                std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                return content;
            }
        }
        // 将 URL 路径拼接到 web 目录下，URL 中的 / 替换为平台路径分隔符
        std::string fpath = g_webDir + SEP;
        for (size_t i = 1; i < urlPath.size(); i++) {
            if (urlPath[i] == '/') fpath += SEP;
            else fpath += urlPath[i];
        }
        std::ifstream f(fpath, std::ios::binary);
        if (f.is_open()) {
            std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
            return content;
        }
        return "";
    }

    // 判断请求是否指向静态文件（路径不以 /api/ 开头的是静态文件）
    static bool isStaticFile(const std::string& path) {
        return path.find("/api/") != 0;
    }

    // ============================================================
    // 【服务器主循环】start —— 启动 HTTP 服务器
    // ============================================================
    // 这是整个程序的网络核心，以下按 WinSock 标准流程逐步讲解
    // ============================================================
    void start(const std::string& webDir) {
        g_webDir = webDir;

        // =====================================================
        // 第1步：初始化网络库（仅 Windows 需要）
        // =====================================================
        // Windows：WSAStartup 初始化 WinSock 库，必须在任何 Socket 操作前调用
        // macOS/Linux：POSIX socket 不需要初始化，直接可用
#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "WSAStartup failed" << std::endl;
            return;
        }
#endif

        // =====================================================
        // 第2步：socket —— 创建套接字（"买一部电话机"）
        // =====================================================
        // 参数说明：
        //   AF_INET      ：使用 IPv4 地址（xxx.xxx.xxx.xxx 格式）
        //   SOCK_STREAM  ：使用 TCP 协议（可靠传输，数据不会丢）
        //   0            ：自动选择 TCP 对应的协议
        // 返回值：套接字句柄（类似文件描述符），INVALID_SOCKET 表示失败
        SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == INVALID_SOCKET) {
            std::cerr << "socket failed" << std::endl;
#ifdef _WIN32
            WSACleanup();
#endif
            return;
        }

        // =====================================================
        // 第3步：bind —— 绑定 IP 地址和端口（"分配电话号码"）
        // =====================================================
        // sockaddr_in 结构体配置：
        //   sin_family      = AF_INET  ：IPv4
        //   sin_addr.s_addr = INADDR_ANY：监听本机所有网卡（0.0.0.0）
        //   sin_port        = htons(8080)：端口号 8080
        //
        // 关于 htons()：
        //   网络中数据按"大端字节序"传输，而 x86/Windows 是"小端"。
        //   htons = Host TO Network Short，将 16 位端口号从小端转为大端
        //   如果不转，数字 8080 在网络上可能被解析为 25375
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(8080);

        if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << "bind failed" << std::endl;
            CLOSE_SOCKET(serverSocket);
#ifdef _WIN32
            WSACleanup();
#endif
            return;
        }

        // =====================================================
        // 第4步：listen —— 进入监听状态（"开机等电话"）
        // =====================================================
        // SOMAXCONN：系统允许的最大等待连接数（通常约 200 个）
        // 此时服务器开始等待客户端（浏览器）的连接请求
        if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
            std::cerr << "listen failed" << std::endl;
            CLOSE_SOCKET(serverSocket);
#ifdef _WIN32
            WSACleanup();
#endif
            return;
        }

        std::cout << "成绩管理系统已启动，请打开浏览器访问 http://localhost:8080" << std::endl;
        std::cout << "默认账号: admin, 密码: 123456" << std::endl;

        // 接收缓冲区：32KB，用于存放客户端发来的 HTTP 请求
        char recvBuf[32768];

        // =====================================================
        // 第5步：accept → recv → send —— 主循环处理请求
        // =====================================================
        // 无限循环，逐个处理客户端的连接
        while (true) {
            // ----- accept：接受一个客户端连接（"拿起听筒"）-----
            // 当没有客户端连接时，accept 会阻塞（卡住等待）
            // 返回一个新的套接字专门用于与该客户端通信
            SOCKET clientSocket = accept(serverSocket, NULL, NULL);
            if (clientSocket == INVALID_SOCKET) continue;

            // ----- recv：接收客户端发来的数据（"听对方说话"）-----
            // TCP 是流协议，一次 recv() 可能只收到部分数据（头部和体可能分两次到达）
            // 因此需要循环接收，直到收到完整请求
            int bytes = recv(clientSocket, recvBuf, sizeof(recvBuf) - 1, 0);
            if (bytes <= 0) { CLOSE_SOCKET(clientSocket); continue; }
            recvBuf[bytes] = '\0';
            std::string rawRequest(recvBuf, bytes);

            // 检查请求体是否接收完整：找到 Content-Length 头部
            size_t clPos = rawRequest.find("Content-Length: ");
            if (clPos != std::string::npos) {
                clPos += 16; // 跳过 "Content-Length: "
                size_t clEnd = rawRequest.find("\r\n", clPos);
                int contentLength = 0;
                if (clEnd != std::string::npos)
                    contentLength = std::stoi(rawRequest.substr(clPos, clEnd - clPos));

                // 计算还需要接收多少字节
                size_t headerEnd = rawRequest.find("\r\n\r\n");
                if (headerEnd != std::string::npos) {
                    int bodyReceived = (int)bytes - (int)headerEnd - 4;
                    // 如果 body 还没收完，继续接收
                    while (bodyReceived < contentLength && bodyReceived >= 0) {
                        int more = recv(clientSocket, recvBuf + bytes,
                                        sizeof(recvBuf) - bytes - 1, 0);
                        if (more <= 0) break;
                        bytes += more;
                        recvBuf[bytes] = '\0';
                        bodyReceived += more;
                    }
                    rawRequest = std::string(recvBuf, bytes);
                }
            }

            // 解析 HTTP 请求
            std::string method, path, queryString, body, token;
            parseRequest(rawRequest, method, path, queryString, body, token);

            std::string responseBody;
            std::string contentType = "application/json; charset=utf-8";

            // ----- 根据请求类型决定如何响应 -----
            if (isStaticFile(path)) {
                // 静态文件请求：读取 HTML/CSS/JS 文件内容返回给浏览器
                responseBody = readStaticFile(path);
                contentType = getMimeType(path);
            } else if (method == "OPTIONS") {
                // =====================================================
                // 【CORS 预检请求】
                // =====================================================
                // 当浏览器发起跨域请求（或使用非简单方法如 PUT/DELETE）
                // 时，浏览器会先发一个 OPTIONS 请求来"询问"服务器是否允许。
                // 这叫做"预检请求"（preflight request）。
                //
                // 此处返回空响应体，后续会在响应头中添加 CORS 头部，
                // 告诉浏览器"任何来源都可以访问"。
                //
                // CORS（跨域资源共享）解决了浏览器的同源策略限制。
                // 同源策略：浏览器默认禁止 JS 向不同域名/端口的服务器发请求。
                // 例如 localhost:3000 的页面不能直接访问 localhost:8080 的 API。
                // CORS 头就是服务器对浏览器的"许可"："你可以跨域访问我"。
                // =====================================================
                responseBody = "";
            } else {
                // API 请求：调用路由处理函数，分发到具体的业务逻辑
                if (g_handler) {
                    responseBody = g_handler(method, path, body, queryString, token);
                } else {
                    responseBody = "{\"success\":false,\"message\":\"no handler\",\"data\":{}}";
                }
            }

            // =====================================================
            // 组装 HTTP 响应（服务器发给浏览器）
            // =====================================================
            // 一个 HTTP 响应的标准格式：
            //   HTTP/1.1 200 OK\r\n              ← 状态行（版本 状态码 原因短语）
            //   Header1: Value1\r\n               ← 响应头
            //   Header2: Value2\r\n
            //   Content-Length: 123\r\n
            //   \r\n                              ← 空行（头与体分隔）
            //   {"success":true,...}              ← 响应体（JSON 数据）
            //
            // 常用状态码：
            //   200 OK          ：请求成功
            //   204 No Content  ：请求成功但没有返回内容
            //   400 Bad Request ：客户端请求有误
            //   401 Unauthorized：未认证（没登录）
            //   404 Not Found   ：资源不存在
            //   500 Internal Server Error：服务器内部错误
            // =====================================================

            // 若有响应体用 200，若为空（OPTIONS或文件未找到）用 204
            std::string statusLine = responseBody.empty() ? "HTTP/1.1 204 No Content\r\n" : "HTTP/1.1 200 OK\r\n";
            std::string response = statusLine;

            // =====================================================
            // 【CORS 响应头详解】
            // =====================================================
            // Access-Control-Allow-Origin: *    ← 允许任何域名的页面访问本服务器
            // Access-Control-Allow-Headers: *    ← 允许请求携带任何自定义头
            // Access-Control-Allow-Methods: *    ← 允许任意 HTTP 方法（GET/POST/PUT/DELETE...）
            //
            // 这三个头协同工作，让前端页面（无论部署在哪）都能调用本服务器 API。
            // 生产环境中通常把 * 替换为具体域名以提高安全性，例如：
            //   Access-Control-Allow-Origin: https://myapp.example.com
            // =====================================================
            response += "Access-Control-Allow-Origin: *\r\n";
            response += "Access-Control-Allow-Headers: *\r\n";
            response += "Access-Control-Allow-Methods: *\r\n";
            // Content-Type：告诉浏览器返回数据的格式
            response += "Content-Type: " + contentType + "\r\n";
            // Content-Length：返回数据的字节数（必须和实际数据长度一致）
            response += "Content-Length: " + std::to_string(responseBody.size()) + "\r\n";
            // Connection: close：每次请求处理完后关闭 TCP 连接（短连接模式）
            response += "Connection: close\r\n";
            response += "\r\n";         // 空行——HTTP 协议规定头体之间必须有空行
            response += responseBody;   // 实际的响应内容

            // ----- send：将响应数据发送给客户端（"向对方说话"）-----
            // response.c_str()：C 风格字符串指针
            // response.size()：要发送的字节数
            // 最后一个参数 0 表示默认行为（阻塞发送）
            send(clientSocket, response.c_str(), (int)response.size(), 0);

            // ----- close：关闭与该客户端的连接（"挂断电话"）-----
            CLOSE_SOCKET(clientSocket);
        }

        // 程序退出时（实际上永远不会执行到这里，因为 while(true) 无限循环）
        CLOSE_SOCKET(serverSocket);
#ifdef _WIN32
        WSACleanup();   // Windows：清理 WinSock 库，释放系统资源
#endif                  // macOS/Linux：不需要清理
    }

}
