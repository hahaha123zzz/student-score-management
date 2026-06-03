#include "server.h"
#include <winsock2.h>
#include <iostream>
#include <fstream>
#include <sstream>

#pragma comment(lib, "ws2_32.lib")

namespace server {

    static RouteHandler g_handler = NULL;
    static std::string g_webDir;

    void setRouteHandler(RouteHandler handler) {
        g_handler = handler;
    }

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

        size_t lineEnd = raw.find("\r\n");
        if (lineEnd == std::string::npos) return;
        std::string requestLine = raw.substr(0, lineEnd);

        size_t space1 = requestLine.find(' ');
        size_t space2 = requestLine.rfind(' ');
        if (space1 == std::string::npos || space2 == std::string::npos) return;
        method = requestLine.substr(0, space1);
        std::string fullPath = requestLine.substr(space1 + 1, space2 - space1 - 1);

        size_t qpos = fullPath.find('?');
        if (qpos != std::string::npos) {
            path = fullPath.substr(0, qpos);
            queryString = fullPath.substr(qpos + 1);
        } else {
            path = fullPath;
        }

        size_t authPos = raw.find("Authorization: Bearer ");
        if (authPos != std::string::npos) {
            authPos += 22;
            size_t authEnd = raw.find("\r\n", authPos);
            if (authEnd != std::string::npos)
                token = raw.substr(authPos, authEnd - authPos);
        }

        size_t bodyStart = raw.find("\r\n\r\n");
        if (bodyStart != std::string::npos) {
            body = raw.substr(bodyStart + 4);
        }
    }

    static std::string getMimeType(const std::string& path) {
        if (path.size() >= 5 && path.substr(path.size() - 5) == ".html") return "text/html; charset=utf-8";
        if (path.size() >= 4 && path.substr(path.size() - 4) == ".css") return "text/css; charset=utf-8";
        if (path.size() >= 3 && path.substr(path.size() - 3) == ".js") return "application/javascript; charset=utf-8";
        if (path.size() >= 4 && path.substr(path.size() - 4) == ".json") return "application/json; charset=utf-8";
        return "text/plain; charset=utf-8";
    }

    static std::string readStaticFile(const std::string& urlPath) {
        if (urlPath == "/") {
            std::string fpath = g_webDir + "\\login.html";
            std::ifstream f(fpath, std::ios::binary);
            if (f.is_open()) {
                std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                return content;
            }
        }
        std::string fpath = g_webDir + "\\";
        for (size_t i = 1; i < urlPath.size(); i++) {
            if (urlPath[i] == '/') fpath += '\\';
            else fpath += urlPath[i];
        }
        std::ifstream f(fpath, std::ios::binary);
        if (f.is_open()) {
            std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
            return content;
        }
        return "";
    }

    static bool isStaticFile(const std::string& path) {
        return path.find("/api/") != 0;
    }

    void start(const std::string& webDir) {
        g_webDir = webDir;

        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "WSAStartup failed" << std::endl;
            return;
        }

        SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == INVALID_SOCKET) {
            std::cerr << "socket failed" << std::endl;
            WSACleanup();
            return;
        }

        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(8080);

        if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << "bind failed" << std::endl;
            closesocket(serverSocket);
            WSACleanup();
            return;
        }

        if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
            std::cerr << "listen failed" << std::endl;
            closesocket(serverSocket);
            WSACleanup();
            return;
        }

        std::cout << "成绩管理系统已启动，请打开浏览器访问 http://localhost:8080" << std::endl;

        char recvBuf[32768];

        while (true) {
            SOCKET clientSocket = accept(serverSocket, NULL, NULL);
            if (clientSocket == INVALID_SOCKET) continue;

            int bytes = recv(clientSocket, recvBuf, sizeof(recvBuf) - 1, 0);
            if (bytes <= 0) {
                closesocket(clientSocket);
                continue;
            }
            recvBuf[bytes] = '\0';
            std::string rawRequest(recvBuf);

            std::string method, path, queryString, body, token;
            parseRequest(rawRequest, method, path, queryString, body, token);

            std::string responseBody;
            std::string contentType = "application/json; charset=utf-8";

            if (isStaticFile(path)) {
                responseBody = readStaticFile(path);
                contentType = getMimeType(path);
            } else if (method == "OPTIONS") {
                responseBody = "";
            } else {
                if (g_handler) {
                    responseBody = g_handler(method, path, body, queryString, token);
                } else {
                    responseBody = "{\"success\":false,\"message\":\"no handler\",\"data\":{}}";
                }
            }

            std::string statusLine = responseBody.empty() ? "HTTP/1.1 204 No Content\r\n" : "HTTP/1.1 200 OK\r\n";
            std::string response = statusLine;
            response += "Access-Control-Allow-Origin: *\r\n";
            response += "Access-Control-Allow-Headers: *\r\n";
            response += "Access-Control-Allow-Methods: *\r\n";
            response += "Content-Type: " + contentType + "\r\n";
            response += "Content-Length: " + std::to_string(responseBody.size()) + "\r\n";
            response += "Connection: close\r\n";
            response += "\r\n";
            response += responseBody;

            send(clientSocket, response.c_str(), (int)response.size(), 0);
            closesocket(clientSocket);
        }

        closesocket(serverSocket);
        WSACleanup();
    }

}
