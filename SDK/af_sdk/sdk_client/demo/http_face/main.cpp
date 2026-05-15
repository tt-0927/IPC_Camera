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
 *   命令交互通过 /api/v1/sdk/command 承载 NET_TV_* 命令名，由设备侧转发到内部业务链路。
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
 * @brief   : 构造 HTTP-SDK 转发命令 JSON
 * @param    {std::string} &strCommand：SDK 命令名，例如 NET_TV_GET_FACECAPTUREINFO
 * @param    {std::string} &strDataJson：业务数据 JSON，传空时使用空对象
 * @return   {std::string} 可发送给 /api/v1/sdk/command 的 JSON 请求体
 */
std::string buildSdkCommandBody(const std::string &strCommand, const std::string &strDataJson = "{}")
{
    const std::string strData = strDataJson.empty() ? "{}" : strDataJson;
    return std::string("{\"Command\":\"") + strCommand + "\",\"Data\":" + strData + "}";
}

/**
 * @brief   : 通过 HTTP-SDK 转发入口发送 SDK 命令
 * @param    {std::string} &strBaseUrl：设备 CGI 基础 URL
 * @param    {std::string} &strCommand：SDK 命令名
 * @param    {std::string} &strDataJson：业务数据 JSON
 * @return   {HttpResult} HTTP 响应结果
 */
HttpResult sendSdkCommand(const std::string &strBaseUrl,
                          const std::string &strCommand,
                          const std::string &strDataJson = "{}")
{
    return sendJsonCommand("POST",
                           strBaseUrl,
                           "/api/v1/sdk/command",
                           buildSdkCommandBody(strCommand, strDataJson));
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
              << " 命令=" << req.form.get_field("Command")
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
              << " 命令=" << req.form.get_field("Command")
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
                    sendSdkCommand(strDeviceBaseUrl, "NET_TV_GET_FACECAPTUREINFO"));

    const std::string strCaptureConfig = R"({
        "Enable": true,
        "Rule": {
            "Interval": 5
        }
    })";
    printHttpResult("设置人脸抓拍配置",
                    sendSdkCommand(strDeviceBaseUrl, "NET_TV_SET_FACECAPTUREINFO", strCaptureConfig));

    printHttpResult("获取人脸比对配置",
                    sendSdkCommand(strDeviceBaseUrl, "NET_TV_GET_FACE_COMPARE_INFO"));

    const std::string strCompareConfig = R"({
        "Enable": true,
        "LinkageSuccessMode": {
            "Tradition": [6, 7],
            "AlarmLinkage": [],
            "RecordChn": []
        },
        "LinkageFailMode": {
            "Tradition": [6],
            "AlarmLinkage": [],
            "RecordChn": []
        }
    })";
    printHttpResult("设置人脸比对配置",
                    sendSdkCommand(strDeviceBaseUrl, "NET_TV_SET_FACE_COMPARE_INFO", strCompareConfig));

    const std::string strAddLib = R"({
        "LibId": "员工库"
    })";
    printHttpResult("添加目标库",
                    sendSdkCommand(strDeviceBaseUrl, "NET_TV_ADD_TARGET_LIB", strAddLib));

    const std::string strAddPerson = R"({
        "LibId": "员工库",
        "Name": "张三",
        "PhoneNum": "13800000000",
        "PicPath": "/opt/cam/face/zhangsan.jpg",
        "BinPath": "",
        "PicType": "jpg",
        "PicSize": 102400,
        "PicDate": "2026-05-14 10:00:00",
        "PicWidth": 640,
        "PicHeight": 480
    })";
    printHttpResult("添加人脸",
                    sendSdkCommand(strDeviceBaseUrl, "NET_TV_ADD_FACE_INFO", strAddPerson));
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
              << "3. 获取人脸比对配置\n"
              << "4. 设置人脸比对配置\n"
              << "5. 添加目标库\n"
              << "6. 删除目标库\n"
              << "7. 修改目标库\n"
              << "8. 获取目标库\n"
              << "9. 添加人脸\n"
              << "10. 删除人脸\n"
              << "11. 修改人脸\n"
              << "12. 获取人脸\n"
              << "13. 自动发送一组示例命令\n"
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
                        sendSdkCommand(strDeviceBaseUrl, "NET_TV_GET_FACECAPTUREINFO"));
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
                        sendSdkCommand(strDeviceBaseUrl, "NET_TV_SET_FACECAPTUREINFO", strBody));
    }
    else if (strChoice == "3")
    {
        printHttpResult("获取人脸比对配置",
                        sendSdkCommand(strDeviceBaseUrl, "NET_TV_GET_FACE_COMPARE_INFO"));
    }
    else if (strChoice == "4")
    {
        const std::string strBody = R"({
            "Enable": true,
            "LinkageSuccessMode": {
                "Tradition": [6, 7],
                "AlarmLinkage": [],
                "RecordChn": []
            },
            "LinkageFailMode": {
                "Tradition": [6],
                "AlarmLinkage": [],
                "RecordChn": []
            }
        })";
        printHttpResult("设置人脸比对配置",
                        sendSdkCommand(strDeviceBaseUrl, "NET_TV_SET_FACE_COMPARE_INFO", strBody));
    }
    else if (strChoice == "5")
    {
        const std::string strBody = R"({
            "LibId": "员工库"
        })";
        printHttpResult("添加目标库",
                        sendSdkCommand(strDeviceBaseUrl, "NET_TV_ADD_TARGET_LIB", strBody));
    }
    else if (strChoice == "6")
    {
        const std::string strBody = R"({
            "LibId": "员工库"
        })";
        printHttpResult("删除目标库",
                        sendSdkCommand(strDeviceBaseUrl, "NET_TV_DEL_TARGET_LIB", strBody));
    }
    else if (strChoice == "7")
    {
        const std::string strBody = R"({
            "LibId_old": "员工库",
            "LibId_new": "员工库-修改"
        })";
        printHttpResult("修改目标库",
                        sendSdkCommand(strDeviceBaseUrl, "NET_TV_SET_TARGET_LIB", strBody));
    }
    else if (strChoice == "8")
    {
        printHttpResult("获取目标库",
                        sendSdkCommand(strDeviceBaseUrl, "NET_TV_GET_TARGET_LIB"));
    }
    else if (strChoice == "9")
    {
        const std::string strBody = R"({
            "LibId": "员工库",
            "Name": "张三",
            "PhoneNum": "13800000000",
            "PicPath": "/opt/cam/face/zhangsan.jpg",
            "BinPath": "",
            "PicType": "jpg",
            "PicSize": 102400,
            "PicDate": "2026-05-14 10:00:00",
            "PicWidth": 640,
            "PicHeight": 480
        })";
        printHttpResult("添加人脸",
                        sendSdkCommand(strDeviceBaseUrl, "NET_TV_ADD_FACE_INFO", strBody));
    }
    else if (strChoice == "10")
    {
        const std::string strBody = R"({
            "Ids": [
                10001
            ]
        })";
        printHttpResult("删除人脸",
                        sendSdkCommand(strDeviceBaseUrl, "NET_TV_DEL_FACE_INFO", strBody));
    }
    else if (strChoice == "11")
    {
        const std::string strBody = R"({
            "Id": 10001,
            "LibId": "员工库",
            "Name": "张三-修改",
            "PhoneNum": "13900000000",
            "PicPath": "/opt/cam/face/zhangsan_new.jpg",
            "PicType": "jpg",
            "PicSize": 120000,
            "PicDate": "2026-05-14 11:00:00"
        })";
        printHttpResult("修改人脸",
                        sendSdkCommand(strDeviceBaseUrl, "NET_TV_SET_FACE_INFO", strBody));
    }
    else if (strChoice == "12")
    {
        const std::string strBody = R"({
            "LibId": "员工库",
            "Name": "",
            "PhoneNum": "",
            "ModelState": -1,
            "RatingLevel": -1
        })";
        printHttpResult("获取人脸",
                        sendSdkCommand(strDeviceBaseUrl, "NET_TV_GET_FACE_INFO", strBody));
    }
    else if (strChoice == "13")
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
