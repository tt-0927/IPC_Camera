/*
 * @FilePath     : CurlHttp.h
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2023-07-18 17:40:51
 * @LastEditors  : yanzeh yanzeh@kfb.cn
 * @LastEditTime : 2023-07-18 19:29:30
 * @Description  :
 */

#pragma once

#include <atomic>
#include <functional>
#include <list>
#include <memory>
#include <sstream>
#include <string>

namespace CurlHttp
{

    /*
        HTTP请求 C++精简封装基类
        使用了libcurl库
    */
    class Base
    {
    public:

        /**
         * @brief 构造函数
         * @param [string&] url: 请求url
         * @return [*] 无
         * @note
         */
        Base(const std::string& url = "");

        /**
         * @brief 一般析构函数
         * @return [*] 无
         * @note
         */
        virtual ~Base();

        /**
         * @brief 发送请求
         * @return [*] 0: 成功  -1: libcurl初始化失败  其他: libcurl的错误返回值
         * @note
         */
        virtual int send_request();

        /**
         * @brief 获取这次请求的返回数据
         * @param [string&] strRecv: 返回的数据
         * @return [*] -1: 请求失败  其他: 数据的长度
         * @note
         */
        virtual int get_recvData(std::string& strRecv);

        /**
         * @brief 设置头信息数据
         * @param [std::list<std::string>&] listHeader: 头信息数据
         * @return [*]
         * @note 链表节点参考 Authorization:xxxxxxxxxx
         */
        void set_header(std::list<std::string>& listHeader);

        /**
         * @brief 设置请求路径
         * @param [string&] strPath: 请求路径
         * @return [*]
         * @note
         */
        void set_path(std::string& strPath);
        void set_path(const char * strPath);

        /**
         * @brief 设置请求参数
         * @param [std::list<std::string>&] listParams: 请求参数
         * @return [*]
         * @note
         */
        void set_params(std::list<std::string>& listParams);

        /**
         * @brief 获取错误码解释
         * @param [int] nCode: 请求错误码
         * @return [*] 错误码解释
         * @note
         */
        static const std::string get_error(int nCode);

    protected:

        /* curl 实例 */
        std::shared_ptr<void> m_curl    = nullptr;
        /* 请求头 */
        std::shared_ptr<void> m_headers = nullptr;
        /* 请求url */
        std::string           m_url;
        /* 请求路径 */
        std::string           m_strPaht;
        /* 请求参数 */
        std::string           m_strParams;

        /* 请求到的数据 */
        std::stringstream m_sstream;

        virtual void update_data() = 0;

    private:

        /* 记录Curl是否初始化全局参数 */
        static bool ms_bGlobalInit;

        /**
         * @brief 回调函数
         * @param [void*] buffer:
         * @param [size_t] size:
         * @param [size_t] nmemb:
         * @param [void*] userp:
         * @return [*]
         * @note
         */
        static size_t Callback(void* buffer, size_t size, size_t nmemb, void* userp);

    private:
        /**
         * @brief 解析带http://前缀的Url地址
         * @param [string] Url地址
         * @return true: 成功， false 失败
         */
        bool parseUrl(const std::string &url);
    };

    /*
        HTTP请求 C++精简封装Post请求模块
        使用了libcurl库
    */
    class Post : public Base
    {
    public:

        /**
         * @brief 构造函数
         * @param [string&] url: 请求url
         * @return [*] 无
         * @note
         */
        Post(const std::string& url);



    private:

        std::string m_toSend;

        void update_data() final;
    };

    /*
        HTTP请求 C++精简封装Get请求模块
        使用了libcurl库
    */
    class Get : public Base
    {
    public:

        /**
         * @brief 构造函数
         * @param [string&] url: 请求url
         * @return [*] 无
         * @note
         */
        Get(const std::string& url);

    private:

        std::string m_urlFinal;

        void update_data() final;

    };

    /*
    HTTP请求 C++精简封装PUT请求模块
    使用了libcurl库
    */
    class PUT : public Base
    {
    public:

        /**
         * @brief 构造函数
         * @param [string&] url: 请求url
         * @return [*] 无
         * @note
         */
        PUT(const std::string& url);

        /* 设置请求json数据 */
        void  set_data(char *pData);

    private:

        /*发送的请求命令*/
        std::string m_putSendtoServer;

        /*发送请求的json数据*/
        std::string m_putSendJson;

        /* 更新请求数据 */
        void update_data() final;

    };

}    // namespace CurlHttp
