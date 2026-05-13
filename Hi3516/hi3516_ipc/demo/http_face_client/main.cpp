/**
 * @FilePath     : main.cpp
 * @Description  : 人脸 HTTP 推送/命令交互客户端示例
 *
 * 编译示例:
 *   g++ -std=c++17 main.cpp -lcurl -pthread -o http_face_client_demo
 *
 * 运行示例:
 *   ./http_face_client_demo http://192.168.1.100/cgi-bin/xxx 18080
 *
 * 说明:
 *   1. HTTP 模式下没有 NET_TV_SetAlarmCallBack 这种 SDK 本地函数回调。
 *   2. “注册回调”等价于平台启动一个 HTTP 接收服务，并把该 URL 配置到设备侧 HTTP 推送配置。
 *   3. 设备侧推送配置示例:
 *      {
 *        "Enable": true,
 *        "CaptureUrl": "http://平台IP:18080/face/event",
 *        "CompareUrl": "http://平台IP:18080/face/event",
 *        "Token": "demo-token"
 *      }
 */

#include <arpa/inet.h>
#include <curl/curl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <thread>

namespace
{
constexpr int RECV_BUFFER_SIZE = 4096;

struct HttpResponse
{
    long nStatus = 0;
    std::string strBody;
};

/**
 * @brief   : libcurl 接收响应回调
 * @param    {char} *pData：响应数据指针
 * @param    {size_t} nSize：单个数据块大小
 * @param    {size_t} nMem：数据块数量
 * @param    {void} *pUser：用户数据
 * @return   {size_t} 已处理字节数
 */
size_t writeCallback(char *pData, size_t nSize, size_t nMem, void *pUser)
{
    auto *pBody = static_cast<std::string *>(pUser);
    const size_t nTotal = nSize * nMem;
    pBody->append(pData, nTotal);
    return nTotal;
}

/**
 * @brief   : 发送 HTTP JSON 请求
 * @param    {std::string} &strMethod：HTTP 方法
 * @param    {std::string} &strUrl：请求 URL
 * @param    {std::string} &strBody：JSON 请求体，GET 可为空
 * @return   {HttpResponse} HTTP 响应
 */
HttpResponse sendJsonRequest(const std::string &strMethod,
                             const std::string &strUrl,
                             const std::string &strBody = "")
{
    HttpResponse stResponse;
    CURL *pCurl = curl_easy_init();
    if (pCurl == nullptr)
    {
        std::cerr << "curl_easy_init failed" << std::endl;
        return stResponse;
    }

    struct curl_slist *pHeaders = nullptr;
    pHeaders = curl_slist_append(pHeaders, "Content-Type: application/json");

    curl_easy_setopt(pCurl, CURLOPT_URL, strUrl.c_str());
    curl_easy_setopt(pCurl, CURLOPT_HTTPHEADER, pHeaders);
    curl_easy_setopt(pCurl, CURLOPT_CUSTOMREQUEST, strMethod.c_str());
    curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(pCurl, CURLOPT_WRITEDATA, &stResponse.strBody);
    curl_easy_setopt(pCurl, CURLOPT_TIMEOUT, 10L);

    if (!strBody.empty())
    {
        curl_easy_setopt(pCurl, CURLOPT_POSTFIELDS, strBody.c_str());
        curl_easy_setopt(pCurl, CURLOPT_POSTFIELDSIZE, static_cast<long>(strBody.size()));
    }

    const CURLcode enRet = curl_easy_perform(pCurl);
    if (enRet != CURLE_OK)
    {
        std::cerr << "HTTP request failed: " << curl_easy_strerror(enRet) << std::endl;
    }
    curl_easy_getinfo(pCurl, CURLINFO_RESPONSE_CODE, &stResponse.nStatus);

    curl_slist_free_all(pHeaders);
    curl_easy_cleanup(pCurl);
    return stResponse;
}

/**
 * @brief   : 从 HTTP 请求头中解析 Content-Length
 * @param    {std::string} &strHeader：HTTP 请求头
 * @return   {int} 请求体长度，未解析到返回 0
 */
int parseContentLength(const std::string &strHeader)
{
    std::istringstream iss(strHeader);
    std::string strLine;
    while (std::getline(iss, strLine))
    {
        if (!strLine.empty() && strLine.back() == '\r')
        {
            strLine.pop_back();
        }

        const std::string strKey = "Content-Length:";
        if (strLine.compare(0, strKey.size(), strKey) == 0)
        {
            return std::atoi(strLine.substr(strKey.size()).c_str());
        }
    }
    return 0;
}

/**
 * @brief   : 打印收到的人脸事件推送
 * @param    {std::string} &strHeader：HTTP 请求头
 * @param    {std::string} &strBody：HTTP 请求体
 * @return   {void}
 */
void printFaceEvent(const std::string &strHeader, const std::string &strBody)
{
    std::cout << "\n========== Face HTTP Event ==========" << std::endl;
    std::cout << strHeader << std::endl;
    std::cout << "BodyLength: " << strBody.size() << std::endl;
    std::cout << "BodyPreview:" << std::endl;
    std::cout << strBody.substr(0, 1024) << std::endl;
    std::cout << "=====================================" << std::endl;
}

class FaceEventHttpServer
{
public:
    /**
     * @brief   : 启动 HTTP 推送接收服务
     * @param    {int} nPort：监听端口
     * @return   {bool} true：启动成功 false：启动失败
     */
    bool start(int nPort)
    {
        m_bRunning.store(true);
        m_thread = std::thread(&FaceEventHttpServer::run, this, nPort);
        return true;
    }

    /**
     * @brief   : 停止 HTTP 推送接收服务
     * @return   {void}
     */
    void stop()
    {
        m_bRunning.store(false);
        const int nFd = m_nListenFd.exchange(-1);
        if (nFd >= 0)
        {
            shutdown(nFd, SHUT_RDWR);
            close(nFd);
        }
        if (m_thread.joinable())
        {
            m_thread.join();
        }
    }

private:
    /**
     * @brief   : HTTP 服务主循环
     * @param    {int} nPort：监听端口
     * @return   {void}
     */
    void run(int nPort)
    {
        const int nListenFd = socket(AF_INET, SOCK_STREAM, 0);
        if (nListenFd < 0)
        {
            std::cerr << "socket failed" << std::endl;
            return;
        }
        m_nListenFd.store(nListenFd);

        int nReuse = 1;
        setsockopt(nListenFd, SOL_SOCKET, SO_REUSEADDR, &nReuse, sizeof(nReuse));

        sockaddr_in stAddr;
        std::memset(&stAddr, 0, sizeof(stAddr));
        stAddr.sin_family = AF_INET;
        stAddr.sin_addr.s_addr = INADDR_ANY;
        stAddr.sin_port = htons(nPort);

        if (bind(nListenFd, reinterpret_cast<sockaddr *>(&stAddr), sizeof(stAddr)) != 0)
        {
            std::cerr << "bind failed, port=" << nPort << std::endl;
            close(nListenFd);
            m_nListenFd.store(-1);
            return;
        }

        if (listen(nListenFd, 16) != 0)
        {
            std::cerr << "listen failed" << std::endl;
            close(nListenFd);
            m_nListenFd.store(-1);
            return;
        }

        std::cout << "Face event callback server listening on 0.0.0.0:" << nPort
                  << "/face/event" << std::endl;

        while (m_bRunning.load())
        {
            sockaddr_in stClientAddr;
            socklen_t nClientLen = sizeof(stClientAddr);
            const int nClientFd = accept(nListenFd,
                                         reinterpret_cast<sockaddr *>(&stClientAddr),
                                         &nClientLen);
            if (nClientFd < 0)
            {
                continue;
            }
            handleClient(nClientFd);
            close(nClientFd);
        }

        const int nFd = m_nListenFd.exchange(-1);
        if (nFd >= 0)
        {
            close(nFd);
        }
    }

    /**
     * @brief   : 处理单个 HTTP 推送连接
     * @param    {int} nClientFd：客户端 socket
     * @return   {void}
     */
    void handleClient(int nClientFd)
    {
        std::string strRequest;
        char szBuffer[RECV_BUFFER_SIZE] = {0};

        while (strRequest.find("\r\n\r\n") == std::string::npos)
        {
            const int nRead = recv(nClientFd, szBuffer, sizeof(szBuffer), 0);
            if (nRead <= 0)
            {
                return;
            }
            strRequest.append(szBuffer, nRead);
        }

        const size_t nHeaderEnd = strRequest.find("\r\n\r\n");
        const std::string strHeader = strRequest.substr(0, nHeaderEnd);
        std::string strBody = strRequest.substr(nHeaderEnd + 4);

        const int nContentLength = parseContentLength(strHeader);
        while (static_cast<int>(strBody.size()) < nContentLength)
        {
            const int nRead = recv(nClientFd, szBuffer, sizeof(szBuffer), 0);
            if (nRead <= 0)
            {
                break;
            }
            strBody.append(szBuffer, nRead);
        }

        printFaceEvent(strHeader, strBody);

        const char *pResp =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Connection: close\r\n"
            "Content-Length: 9\r\n"
            "\r\n"
            "{\"Ret\":0}";
        send(nClientFd, pResp, std::strlen(pResp), 0);
    }

private:
    std::atomic<bool> m_bRunning{false};
    std::atomic<int> m_nListenFd{-1};
    std::thread m_thread;
};

/**
 * @brief   : 拼接 URL
 * @param    {std::string} &strBase：基础 URL
 * @param    {std::string} &strPath：接口路径
 * @return   {std::string} 拼接后的 URL
 */
std::string joinUrl(const std::string &strBase, const std::string &strPath)
{
    if (!strBase.empty() && strBase.back() == '/')
    {
        return strBase.substr(0, strBase.size() - 1) + strPath;
    }
    return strBase + strPath;
}

/**
 * @brief   : 发送人脸相关正常命令示例
 * @param    {std::string} &strDeviceBaseUrl：设备 CGI 基础 URL
 * @return   {void}
 */
void sendCommandExamples(const std::string &strDeviceBaseUrl)
{
    {
        const std::string strUrl = joinUrl(strDeviceBaseUrl, "/api/v1/face/capture/config");
        const HttpResponse stResp = sendJsonRequest("GET", strUrl);
        std::cout << "\nGET capture config status=" << stResp.nStatus
                  << "\n" << stResp.strBody << std::endl;
    }

    {
        const std::string strUrl = joinUrl(strDeviceBaseUrl, "/api/v1/face/capture/config");
        const std::string strBody = R"({
            "Enable": true,
            "Rule": {
                "Interval": 5
            }
        })";
        const HttpResponse stResp = sendJsonRequest("PUT", strUrl, strBody);
        std::cout << "\nPUT capture config status=" << stResp.nStatus
                  << "\n" << stResp.strBody << std::endl;
    }

    {
        const std::string strUrl = joinUrl(strDeviceBaseUrl, "/api/v1/face/libs");
        const std::string strBody = R"({
            "LibName": "员工库"
        })";
        const HttpResponse stResp = sendJsonRequest("POST", strUrl, strBody);
        std::cout << "\nPOST face lib status=" << stResp.nStatus
                  << "\n" << stResp.strBody << std::endl;
    }

    {
        const std::string strUrl = joinUrl(strDeviceBaseUrl, "/api/v1/face/persons");
        const std::string strBody = R"({
            "FaceLibID": 1,
            "Name": "张三",
            "PicPath": "/opt/cam/face/zhangsan.jpg"
        })";
        const HttpResponse stResp = sendJsonRequest("POST", strUrl, strBody);
        std::cout << "\nPOST face person status=" << stResp.nStatus
                  << "\n" << stResp.strBody << std::endl;
    }
}
} // namespace

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::cout << "Usage: " << argv[0]
                  << " <device-cgi-base-url> <callback-port>\n"
                  << "Example: " << argv[0]
                  << " http://192.168.1.100/cgi-bin/xxx 18080\n";
        return 0;
    }

    const std::string strDeviceBaseUrl = argv[1];
    const int nCallbackPort = std::atoi(argv[2]);

    curl_global_init(CURL_GLOBAL_ALL);

    FaceEventHttpServer stServer;
    stServer.start(nCallbackPort);

    std::cout << "\n请先把设备侧 HTTP 推送地址配置为: "
              << "http://平台IP:" << nCallbackPort << "/face/event\n"
              << "然后触发人脸抓拍/比对事件，即可在本程序看到推送。\n";

    sendCommandExamples(strDeviceBaseUrl);

    std::cout << "\nPress Enter to exit..." << std::endl;
    std::cin.get();

    stServer.stop();
    curl_global_cleanup();
    return 0;
}
