/*** 
 * @FilePath     : ProductionUploader.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-08 11:25:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-08 11:25:00
 * @Description  : 产测结果上传
 */

#pragma once

#include <string>

class ProductionUploader
{
public:
    /**
     * @brief  上传产测结果到运维平台
     * @param  strJsonData JSON格式的测试数据
     * @return int 0成功，非0失败
     */
    static int upload(const std::string &strJsonData);

private:
    /**
     * @brief  登录运维平台获取token
     * @param  strToken 输出参数，返回token
     * @return int 0成功，非0失败
     */
    static int doLogin(std::string &strToken);

    /**
     * @brief  上传文件到运维平台
     * @param  strToken API token
     * @param  strFilePath 本地文件路径
     * @param  strFileName 文件名
     * @return int 0成功，非0失败
     */
    static int doUploadFile(const std::string &strToken,
                            const std::string &strFilePath,
                            const std::string &strFileName);

    /**
     * @brief  获取设备MAC地址
     * @return std::string MAC地址字符串
     */
    static std::string getMac();
};
