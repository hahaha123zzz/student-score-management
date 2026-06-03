#pragma once

#include <string>

namespace server {

    void start(const std::string& webDir);

    // 路由处理函数类型
    typedef std::string (*RouteHandler)(const std::string& path, const std::string& body,
                                         const std::string& queryString, const std::string& token);

    void setRouteHandler(RouteHandler handler);

}
