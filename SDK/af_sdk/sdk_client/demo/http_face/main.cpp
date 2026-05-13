/**
 * @FilePath     : main.cpp
 * @Description  : HTTP 人脸推送与命令交互 Demo
 *
 * 编译示例:
 *   mkdir -p build && cd build
 *   cmake ..
 *   cmake --build .
 *
 * 运行示例:
 *   ./HttpFaceClientDemo http://192.168.1.100/cgi-bin/xxx 18080
 *
 * 说明:
 *   HTTP 模式不走 NET_TV_Login / NET_TV_SetAlarmCallBack。
 *   “注册回调”等价于平台启动 HTTP 服务，并把服务地址配置到设备侧 HTTP 推送配置中。
 */

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

#include "tvsdkhttplib.h"

namespace httplib = tvsdk::httplib;

namespace
{
/**
 * @brief   : HTTP 响应结果
 */
struct HttpResult
{
    int nStatus = 0;
    std::string strBody;
};

/**
 * @brief   : 去除 URL 末尾斜杠
 * @param    {std::string} strUrl：原始 URL
 * @return   {std::string} 去除末尾斜杠后的 URL
 */
std::string trimTrailingSlash(std::string strUrl)
{
    while (!strUrl.empty() && strUrl.back() == '/')
    {
        strUrl.pop_back();
    }
    return strUrl;
}

/**
 * @brief   : 打印 HTTP 命令响应
 * @param    {std::string} &strTitle：命令标题
 * @param    {HttpResult} &stResult：HTTP 响应结果
 * @return   {void}
 */
void printHttpResult(const std::string &strTitle, const HttpResult &stResult)
{
    std::cout << "\n[" << strTitle << "] HTTP状态码=" << stResult.nStatus << std::endl;
    std::cout << stResult.strBody << std::endl;
}

/**
 * @brief   : 发送 HTTP JSON 命令
 * @param    {std::string} &strMethod：HTTP 方法
 * @param    {std::string} &strBaseUrl：设备 CGI 基础 URL
 * @param    {std::string} &strPath：接口路径
 * @param    {std::string} &strBody：JSON 请求体，GET 可为空
 * @return   {HttpResult} HTTP 响应结果
 */
HttpResult sendJsonCommand(const std::string &strMethod,
                           const std::string &strBaseUrl,
                           const std::string &strPath,
                           const std::string &strBody = "")
{
    HttpResult stResult;

    httplib::Client cli(trimTrailingSlash(strBaseUrl));
    cli.set_connection_timeout(5, 0);
    cli.set_read_timeout(10, 0);
    cli.set_write_timeout(10, 0);

    httplib::Headers headers = {
        {"Content-Type", "application/json"}
    };

    httplib::Result res;
    if (strMethod == "GET")
    {
        res = cli.Get(strPath.c_str(), headers);
    }
    else if (strMethod == "POST")
    {
        res = cli.Post(strPath.c_str(), headers, strBody, "application/json");
    }
    else if (strMethod == "PUT")
    {
        res = cli.Put(strPath.c_str(), headers, strBody, "application/json");
    }
    else if (strMethod == "DELETE")
    {
        res = cli.Delete(strPath.c_str(), headers, strBody, "application/json");
    }
    else
    {
        std::cerr << "不支持的HTTP方法: " << strMethod << std::endl;
        return stResult;
    }

    if (!res)
    {
        std::cerr << "HTTP请求失败, 方法=" << strMethod
                  << " 路径=" << strPath
                  << " 错误码=" << static_cast<int>(res.error()) << std::endl;
        return stResult;
    }

    stResult.nStatus = res->status;
    stResult.strBody = res->body;
    return stResult;
}

/**
 * @brief   : 打印 multipart 普通字段
 * @param    {httplib::Request} &req：HTTP 请求
 * @return   {void}
 */
void printMultipartFields(const httplib::Request &req)
{
    for (const auto &item : req.form.fields)
    {
        std::cout << "  字段 " << item.first << " = " << item.second.content << std::endl;
    }
}

/**
 * @brief   : 打印 multipart 文件字段
 * @param    {httplib::Request} &req：HTTP 请求
 * @return   {void}
 */
void printMultipartFiles(const httplib::Request &req)
{
    for (const auto &item : req.form.files)
    {
        const httplib::FormData &file = item.second;
        std::cout << "  文件 " << item.first
                  << " 文件名=" << file.filename
                  << " 类型=" << file.content_type
                  << " 大小=" << file.content.size()
                  << std::endl;
    }
}

/**
 * @brief   : 判断当前推送是否为人脸比对事件
 * @param    {httplib::Request} &req：HTTP 请求
 * @return   {bool} true：人脸比对 false：非人脸比对
 */
bool isFaceCompareEvent(const httplib::Request &req)
{
    return req.form.has_field("EventType") &&
           req.form.get_field("EventType") == "FACE_COMPARE";
}

/**
 * @brief   : 判断当前推送是否为人脸抓拍事件
 * @param    {httplib::Request} &req：HTTP 请求
 * @return   {bool} true：人脸抓拍 false：非人脸抓拍
 */
bool isFaceCaptureEvent(const httplib::Request &req)
{
    return req.form.has_field("EventType") &&
           req.form.get_field("EventType") == "FACE_CAPTURE";
}

/**
 * @brief   : 打印人脸比对推送摘要
 * @param    {httplib::Request} &req：HTTP 请求
 * @return   {void}
 */
void printFaceCompareSummary(const httplib::Request &req)
{
    std::cout << "  [人脸比对 FACE_COMPARE]"
              << " 结果=" << req.form.get_field("CompareResult")
              << " 人员ID=" << req.form.get_field("FaceID")
              << " 姓名=" << req.form.get_field("FaceName")
              << " 库名称=" << req.form.get_field("FaceLibName")
              << " 相似度=" << req.form.get_field("SimilarityPercent")
              << std::endl;
}

/**
 * @brief   : 打印人脸抓拍推送摘要
 * @param    {httplib::Request} &req：HTTP 请求
 * @return   {void}
 */
void printFaceCaptureSummary(const httplib::Request &req)
{
    std::cout << "  [人脸抓拍 FACE_CAPTURE]"
              << " 目标数=" << req.form.get_field("TargetCount")
              << " 通道=" << req.form.get_field("Channel")
              << std::endl;
}

/**
 * @brief   : 启动 HTTP 推送回调服务
 * @param    {int} nListenPort：监听端口
 * @param    {httplib::Server} &server：HTTP 服务实例
 * @return   {void}
 */
void startCallbackServer(int nListenPort, httplib::Server &server)
{
    server.Post("/face/event", [](const httplib::Request &req, httplib::Response &res) {
        std::cout << "\n========== 收到人脸HTTP推送 ==========" << std::endl;
        std::cout << "来源地址=" << req.remote_addr << ":" << req.remote_port << std::endl;
        std::cout << "Content-Type=" << req.get_header_value("Content-Type") << std::endl;

        if (req.is_multipart_form_data())
        {
            if (isFaceCompareEvent(req))
            {
                printFaceCompareSummary(req);
            }
            else if (isFaceCaptureEvent(req))
            {
                printFaceCaptureSummary(req);
            }
            printMultipartFields(req);
            printMultipartFiles(req);
        }
        else
        {
            std::cout << req.body << std::endl;
        }

        std::cout << "====================================" << std::endl;
        res.set_content("{\"Ret\":0}", "application/json");
    });

    server.Get("/health", [](const httplib::Request &, httplib::Response &res) {
        res.set_content("OK", "text/plain");
    });

    std::cout << "HTTP人脸推送回调监听: http://0.0.0.0:"
              << nListenPort << "/face/event" << std::endl;
    server.listen("0.0.0.0", nListenPort);
}

/**
 * @brief   : 发送设备 HTTP 命令示例
 * @param    {std::string} &strDeviceBaseUrl：设备 CGI 基础 URL
 * @return   {void}
 */
void sendCommandExamples(const std::string &strDeviceBaseUrl)
{
    printHttpResult("获取人脸抓拍配置",
                    sendJsonCommand("GET",
                                    strDeviceBaseUrl,
                                    "/api/v1/face/capture/config"));

    const std::string strCaptureConfig = R"({
        "Enable": true,
        "Rule": {
            "Interval": 5
        }
    })";
    printHttpResult("设置人脸抓拍配置",
                    sendJsonCommand("PUT",
                                    strDeviceBaseUrl,
                                    "/api/v1/face/capture/config",
                                    strCaptureConfig));

    const std::string strAddLib = R"({
        "LibName": "员工库"
    })";
    printHttpResult("添加目标库",
                    sendJsonCommand("POST",
                                    strDeviceBaseUrl,
                                    "/api/v1/face/libs",
                                    strAddLib));

    const std::string strAddPerson = R"({
        "FaceLibID": 1,
        "Name": "张三",
        "PicPath": "/opt/cam/face/zhangsan.jpg"
    })";
    printHttpResult("添加人脸",
                    sendJsonCommand("POST",
                                    strDeviceBaseUrl,
                                    "/api/v1/face/persons",
                                    strAddPerson));
}

/**
 * @brief   : 打印交互式命令菜单
 * @return   {void}
 */
void printCommandMenu()
{
    std::cout << "\n========== HTTP人脸命令菜单 ==========\n"
              << "1. 获取人脸抓拍配置\n"
              << "2. 设置人脸抓拍配置\n"
              << "3. 添加目标库\n"
              << "4. 删除目标库\n"
              << "5. 修改目标库\n"
              << "6. 获取目标库\n"
              << "7. 添加人脸\n"
              << "8. 删除人脸\n"
              << "9. 修改人脸\n"
              << "10. 获取人脸\n"
              << "11. 自动发送一组示例命令\n"
              << "q. 退出\n"
              << "请输入命令编号: ";
}

/**
 * @brief   : 根据菜单编号发送指定 HTTP 命令
 * @param    {std::string} &strDeviceBaseUrl：设备 HTTP 基础地址
 * @param    {std::string} &strChoice：用户输入的菜单编号
 * @return   {bool} true：继续运行 false：退出
 */
bool handleMenuChoice(const std::string &strDeviceBaseUrl, const std::string &strChoice)
{
    if (strChoice == "q" || strChoice == "Q")
    {
        return false;
    }

    if (strChoice == "1")
    {
        printHttpResult("获取人脸抓拍配置",
                        sendJsonCommand("GET", strDeviceBaseUrl, "/api/v1/face/capture/config"));
    }
    else if (strChoice == "2")
    {
        const std::string strBody = R"({
            "Enable": true,
            "Rule": {
                "Interval": 5
            }
        })";
        printHttpResult("设置人脸抓拍配置",
                        sendJsonCommand("PUT", strDeviceBaseUrl, "/api/v1/face/capture/config", strBody));
    }
    else if (strChoice == "3")
    {
        const std::string strBody = R"({
            "LibName": "员工库"
        })";
        printHttpResult("添加目标库",
                        sendJsonCommand("POST", strDeviceBaseUrl, "/api/v1/face/libs", strBody));
    }
    else if (strChoice == "4")
    {
        const std::string strBody = R"({
            "FaceLibID": 1
        })";
        printHttpResult("删除目标库",
                        sendJsonCommand("DELETE", strDeviceBaseUrl, "/api/v1/face/libs", strBody));
    }
    else if (strChoice == "5")
    {
        const std::string strBody = R"({
            "FaceLibID": 1,
            "LibName": "员工库-修改"
        })";
        printHttpResult("修改目标库",
                        sendJsonCommand("PUT", strDeviceBaseUrl, "/api/v1/face/libs", strBody));
    }
    else if (strChoice == "6")
    {
        printHttpResult("获取目标库",
                        sendJsonCommand("GET", strDeviceBaseUrl, "/api/v1/face/libs"));
    }
    else if (strChoice == "7")
    {
        const std::string strBody = R"({
            "FaceLibID": 1,
            "Name": "张三",
            "PicPath": "/opt/cam/face/zhangsan.jpg"
        })";
        printHttpResult("添加人脸",
                        sendJsonCommand("POST", strDeviceBaseUrl, "/api/v1/face/persons", strBody));
    }
    else if (strChoice == "8")
    {
        const std::string strBody = R"({
            "FaceID": 10001
        })";
        printHttpResult("删除人脸",
                        sendJsonCommand("DELETE", strDeviceBaseUrl, "/api/v1/face/persons", strBody));
    }
    else if (strChoice == "9")
    {
        const std::string strBody = R"({
            "FaceID": 10001,
            "FaceLibID": 1,
            "Name": "张三-修改",
            "PicPath": "/opt/cam/face/zhangsan_new.jpg"
        })";
        printHttpResult("修改人脸",
                        sendJsonCommand("PUT", strDeviceBaseUrl, "/api/v1/face/persons", strBody));
    }
    else if (strChoice == "10")
    {
        printHttpResult("获取人脸",
                        sendJsonCommand("GET", strDeviceBaseUrl, "/api/v1/face/persons"));
    }
    else if (strChoice == "11")
    {
        sendCommandExamples(strDeviceBaseUrl);
    }
    else
    {
        std::cout << "未知命令编号: " << strChoice << std::endl;
    }

    return true;
}

/**
 * @brief   : 运行交互式命令循环
 * @param    {std::string} &strDeviceBaseUrl：设备 HTTP 基础地址
 * @return   {void}
 */
void runCommandLoop(const std::string &strDeviceBaseUrl)
{
    std::string strChoice;
    while (true)
    {
        printCommandMenu();
        if (!std::getline(std::cin, strChoice))
        {
            break;
        }

        if (!handleMenuChoice(strDeviceBaseUrl, strChoice))
        {
            break;
        }
    }
}
} // namespace

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::cout << "用法: " << argv[0] << " <设备HTTP基础地址> <本地回调监听端口>\n"
                  << "示例: " << argv[0] << " http://192.168.1.100:9000 18080\n";
        return 0;
    }

    const std::string strDeviceBaseUrl = argv[1];
    const int nListenPort = std::atoi(argv[2]);

    httplib::Server server;
    std::thread serverThread(startCallbackServer, nListenPort, std::ref(server));

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    std::cout << "\n请将设备侧 HTTP 推送地址配置为: http://平台IP:"
              << nListenPort << "/face/event\n";
    std::cout << "然后触发人脸抓拍/比对事件，本 Demo 会在控制台打印 multipart 字段和图片大小。\n";

    sendCommandExamples(strDeviceBaseUrl);
    runCommandLoop(strDeviceBaseUrl);

    server.stop();
    if (serverThread.joinable())
    {
        serverThread.join();
    }
    return 0;
}
