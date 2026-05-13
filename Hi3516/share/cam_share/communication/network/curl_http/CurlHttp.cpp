/*
 * @FilePath     : CurlHttp.cpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2023-07-18 17:40:56
 * @LastEditors  : yanzeh yanzeh@kfb.cn
 * @LastEditTime : 2023-09-25 17:13:59
 * @Description  :
 */

#include "CurlHttp.h"

#include "curl.h"

#include <iostream>
#include <cstring>


#ifdef _WIN32
    #pragma comment(lib, "Ws2_32.lib")
#endif

using namespace CurlHttp;
using namespace std;

bool Base::ms_bGlobalInit = false;

Base::Base(const string& url)
    : m_url(url)
{
    /* 如果传输进来的是全路径 */
    parseUrl(url);

    if (!ms_bGlobalInit)
    {
        ms_bGlobalInit = true;
        curl_global_init(CURL_GLOBAL_ALL);
    }

    m_curl = std::shared_ptr<void>(curl_easy_init(), [](void* o) {
        if (o)
        {
            curl_easy_cleanup(o);
        }
    });
    if (!m_curl)
    {
        return;
    }

    curl_easy_setopt(m_curl.get(), CURLOPT_WRITEFUNCTION, Callback);
    /* 指定接收响应数据的位置 */
    curl_easy_setopt(m_curl.get(), CURLOPT_WRITEDATA, &m_sstream);
    /* 将 SSL 主机名验证设置为不验证（关闭验证） */
    curl_easy_setopt(m_curl.get(), CURLOPT_SSL_VERIFYHOST, 0L);
    /* 将 SSL 证书验证设置为不验证（关闭验证） */
    curl_easy_setopt(m_curl.get(), CURLOPT_SSL_VERIFYPEER, 0L);
    /* 控制整个操作的超时时间 */
    curl_easy_setopt(m_curl.get(), CURLOPT_TIMEOUT, 10);
    /* 控制建立连接的超时时间 */
    curl_easy_setopt(m_curl.get(), CURLOPT_CONNECTTIMEOUT, 5);
    /* 发起网络请求时使用 gzip 压缩 */
    curl_easy_setopt(m_curl.get(), CURLOPT_ACCEPT_ENCODING, "gzip");
    /* 输出详细的网络操作信息 */
    curl_easy_setopt(m_curl.get(), CURLOPT_VERBOSE, 1L);
}

Base::~Base()
{
}

/* 发送请求 */
int Base::send_request()
{
    if (!m_curl)
    {
        return -1;
    }

    update_data();

    m_sstream.str("");

    int nRet = -1;
    for (int i = 0; (i < 5) && (nRet != 0); i++)
    {
        nRet = curl_easy_perform(m_curl.get());
        if (CURLE_OK == nRet)
        {
            break;
        }
        else
        {
            //dlog(LOG_ERROR, "http请求失败 错误码[%d] URL[http://%s%s?%s]",
                 //nRet, m_url.c_str(), m_strPaht.c_str(), m_strParams.c_str());
        }
    }

    return nRet;
}

/* 获取这次请求的返回数据 */
int Base::get_recvData(string& strRecv)
{
    if (!m_curl)
    {
        return -1;
    }

    strRecv = m_sstream.str();

    return strRecv.size();
}

/* 设置头数据 */
void Base::set_header(std::list<std::string>& listHeader)
{
    curl_slist* pHeaders = NULL;

    if (!listHeader.empty())
    {
        for (auto item : listHeader)
        {
            pHeaders = curl_slist_append(pHeaders, item.c_str());
        }

        curl_easy_setopt(m_curl.get(), CURLOPT_HTTPHEADER, pHeaders);
    }

    m_headers.reset(pHeaders, curl_slist_free_all);
}

/* 设置请求路径 */
/**
 * @brief 设置请求路径
 * @param [string&] strPath: 请求路径
 * @return [*]
 * @note
 */
void CurlHttp::Base::set_path(std::string& strPath)
{
    if(strPath.rfind("http://", 0) == 0)
    {
        /* 默认不是全路径 */
        m_url = strPath;
        /* 检查是不是全路径 */
        parseUrl(strPath);
    }
    else
    {
        m_strPaht = strPath;
    }
}

void Base::set_path(const char *strPath)
{
    std::string strTmp = strPath;
    set_path(strTmp);
}

/* 设置请求参数 */
void CurlHttp::Base::set_params(std::list<std::string>& listParams)
{
    m_strParams.clear();

    if (listParams.empty())
    {
        return;
    }

    for (auto item : listParams)
    {
        m_strParams += item + "&";
    }

    m_strParams.erase(m_strParams.length() - 1);
}

/* 获取错误码解释 */
const string Base::get_error(int nCode)
{
    return curl_easy_strerror(static_cast<CURLcode>(nCode));
}

/* 回调函数 */
size_t Base::Callback(void* buffer, size_t size, size_t nmemb, void* userp)
{
    string get = string((char*)buffer, size * nmemb);
    *(stringstream*)userp << get;
    return size * nmemb;
}

bool Base::parseUrl(const std::string &url)
{
    if(url.rfind("http://", 0) == 0)
    {
        std::string::size_type index = url.find_first_of('/', strlen("http://\0"));
        if(index != std::string::npos)
        {
            std::string tempPath = url.substr(index);
            if(tempPath.size() > 2)
            {
                m_url = url.substr(0, index);
                m_strPaht = tempPath;
                return true;
            }
        }
    }
    return false;
}

Post::Post(const string& url)
    : Base(url)
{
    std::string strFullUri = m_url + m_strPaht;
    curl_easy_setopt(m_curl.get(), CURLOPT_URL, strFullUri.c_str());
    curl_easy_setopt(m_curl.get(), CURLOPT_POST, 1L);
}

/* 更新请求数据 */
void Post::update_data()
{
    if(m_url.rfind("http://", 0) == 0)
    {
        m_toSend = m_url + m_strPaht + "?" + m_strParams;
    }
    else
    {
        m_toSend = "http://" + m_url + m_strPaht + "?" + m_strParams;
    }

    curl_easy_setopt(m_curl.get(), CURLOPT_POSTFIELDS, m_toSend.c_str());
}

Get::Get(const string& url)
    : Base(url)
{
}

/* 更新请求数据 */
void Get::update_data()
{
    if(m_url.rfind("http://", 0) == 0)
    {
        m_urlFinal = m_url + m_strPaht + "?" + m_strParams;
    }
    else
    {
        m_urlFinal = "http://" + m_url + m_strPaht + "?" + m_strParams;
    }

    curl_easy_setopt(m_curl.get(), CURLOPT_URL, m_urlFinal.c_str());
}

PUT::PUT(const string& url)
    : Base(url)
{
    /*设置PUT请求*/
    curl_easy_setopt(m_curl.get(), CURLOPT_CUSTOMREQUEST, "PUT");
}

/* 更新请求数据 */
void PUT::update_data()
{
    if(m_url.rfind("http://", 0) == 0)
    {
        m_putSendtoServer = m_url + m_strPaht;
    }
    else
    {
        m_putSendtoServer = "http://" + m_url + m_strPaht;
    }

    /*发送请求命令*/
    curl_easy_setopt(m_curl.get(), CURLOPT_URL, m_putSendtoServer.c_str());
    /*发送请求json数据*/
    curl_easy_setopt(m_curl.get(), CURLOPT_POSTFIELDS, m_putSendJson.c_str());
}

/* 设置请求json数据 */
void PUT::set_data(char *pData)
{
    /* 拷贝json数据 */
    m_putSendJson = pData;
}
