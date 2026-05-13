/*
 * @FilePath     : CurlMultipartHttpPost.h
 * @Author       : xiezhh
 * @Date         : 2024-06-04 17:23
 * @LastEditors  : xiezhh
 * @LastEditTime : 2024-06-25 15:24
 * @Description  : 用于Http Post请求，它支持 multipart/formdata 格式
 *                 所有传输的内容（除二进制）都必须转成char *添加进入form中
 *                 注意：传输二进制文件时需要保证，等待请求返回后才能够释放其中的内存。
 */
#ifndef CCURLMULTIPARTHTTPPOST_H
#define CCURLMULTIPARTHTTPPOST_H

#include "CurlHttp.h"
#include <curl.h>

namespace CurlHttp {

    class CCurlMultipartHttpPost : public Base
    {
    public:
        /**
         * @brief CCurlMultipartHttpPost
         * @param [string] 请求的Url
         * @note 注意：Base基类构造函数中解析该url，
         *      如果使用这个类建议传递全路径的请求Url
         */
        CCurlMultipartHttpPost(const std::string& url);

        /* 往表单中添加数字型 */
        /**
         * @brief 往表单中添加int类型字段
         * @param [string] 字段名称
         * @param [int] 字段值
         */
        void add_formData(const std::string &strItem, const int &nData);
        /**
         * @brief 往表单中添加float类型字段
         * @param [string] 字段名称
         * @param [float] 字段值
         */
        void add_formData(const std::string &strItem, const float &fData);
        /**
         * @brief 往表单中添加double类型字段
         * @param [string] 字段名称
         * @param [double] 字段值
         */
        void add_formData(const std::string &strItem, const double &dData);
        /**
         * @brief 往表单中添加std::size_t类型字段
         * @param [string] 字段名称
         * @param [std::size_t] 字段值
         */
        void add_formData(const std::string &strItem, const std::size_t &nData);


        /* 往表单中添加字符串（常用） */
        /**
         * @brief 往表单中添加const char *类型字段
         * @param [string] 字段名称
         * @param [const char *] 字段值
         */
        void add_formData(const std::string &strItem, const char *pData);
        /**
         * @brief 往表单中添加string类型字段
         * @param [string] 字段名称
         * @param [string] 字段值
         */
        void add_formData(const std::string &strItem, const std::string &strData);


        /* 往表单中添加数组型 */
        /**
         * @brief 往表单中添加const int数组类型字段
         * @param [string] 字段名称
         * @param [const int[]] 字段值
         */
        void add_formData(const std::string &strItem, const int arrData[]);
        /**
         * @brief 往表单中添加const char*数组类型字段
         * @param [string] 字段名称
         * @param [int] 数组数据条数
         * @param [const char*[]] 字段值
         */
        void add_formData(const std::string &strItem, int nArrCount, const char *arrData[]);
        /**
         * @brief 往表单中添加tring数组类型字段
         * @param [string] 字段名称
         * @param [const string[]] 字段值
         */
        void add_formData(const std::string &strItem, const std::string arrData[]);


        /* 往表单中添加二进制数据 */
        /**
         * @brief 往表单中添加二进制类型字段
         * @param [string] 字段名称
         * @param [string] 文件名称/Buffer名称
         * @param [void *] 二进制数据Buffer
         * @param [int] Buffer的大小
         */
        void add_formData(const std::string &strItem, const std::string &strFileName, void *buffer, int size);
        /**
         * @brief 往表单中添加二进制类型字段
         * @param [string] 字段名称
         * @param [string] 文件名称/Buffer名称
         * @param [void *] 二进制数据Buffer
         * @param [size_t] Buffer的大小
         */
        void add_formData(const std::string &strItem, const std::string &strFileName, void *buffer, std::size_t size);


        /* 往表单中添加文件 */
        /**
         * @brief 往表单中添加文件类型，二进制字段
         * @param [string] 字段名称
         * @param [string] 文件的绝对路径
         * @note 不需要分片的文件就可以使用该函数上传
         */
        void add_file_formData(const std::string &strItem, const std::string strFileName);

        /**
         * @brief 清空表单
         * @note 清空表单的同时将m_pFirst和m_pLast置为NULL
         */
        void clear_form();

        /**
         * @brief 更新参数
         * @note  从新拼接请求Url
         */
        virtual void update_data() override;

        /**
         * @brief 发送请求
         * @return 不等于CURLE_OK都是失败了
         * @note 失败可以通过get_error函数获取失败信息
         */
        virtual int send_request() override;
    private:
        /* multipart/formdata 链表头和链表尾，由curl进行维护
           在每次重新设置的时候，都需要将其置为NULL
         */
        struct curl_httppost *m_pFirst = NULL;
        struct curl_httppost *m_pLast = NULL;
    };

}

#endif // CCURLMULTIPARTHTTPPOST_H
