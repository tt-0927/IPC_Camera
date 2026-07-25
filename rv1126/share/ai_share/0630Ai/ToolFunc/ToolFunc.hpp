/*
 * @FilePath     : ToolFunc.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-04-01 16:55:16
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-07-30 17:26:17
 * @Description  :
 */
#pragma once

#include <iostream>
#include <sstream>

class ToolFunc
{
public:

    /**
     * @brief 创建目录
     * @param [string&] strPath: 目录路径
     * @return [*]
     * @note 创建当前目录及其所有父目录（如果不存在的话）
     */
    static bool makeDirectory(std::string strPath);

    /**
     * @brief 删除目录
     * @param [string&] strPath: 目录路径
     * @return [*]
     * @note 创建当前目录及其所有父目录（如果不存在的话）
     */
    static bool rmDirectory(std::string strPath);

    /**
     * @brief 打开文件并清空原有的数据，写入文件
     * @param [char*] pchFileName: 文件名
     * @param [void*] pData: 写入的数据
     * @param [size_t] nDataSize: 写入的数据大小
     * @return [*]
     * @note
     */
    static bool writeDataToFile(const char* pchFileName, const void* pData, size_t nDataSize);

    /**
     * @brief 调整图片大小
     * @param [string&] strSrcImagePath: 原始图片路径
     * @param [string&] strDstImagePath: 目标图片路径
     * @param [int] nTargetWidth: 目标图片宽度
     * @param [int] nTargetHeight: 目标图片高度
     * @return [*]
     * @note 用填充灰度颜色的方式
     */
    static bool resizeImage(const std::string& strSrcImagePath, const std::string& strDstImagePath, int nTargetWidth, int nTargetHeight);


    /**
     * @brief 获取微秒级时间戳
     * @return long long
     */
    static long long getTimeStampUs();

    /**
     * @brief 将Json数据写入文件中
     * @param pchFilePath 文件路径
     * @param pchJsonData 需要写入的数据
     * @return bool
     */
    static bool writeJson_to_file(const char* pchFilePath, const char* pchJsonData);

    /**
     * @brief 读取文件中的Json数据
     * @param pchFilePath 文件路径
     * @return char*
     */
    static char* readJson_from_file(const char* pchFilePath);

    /* 拼接字符串 */
    template<typename... Args>
    static std::string toString(Args&&... args)
    {
        std::ostringstream oss;
        (oss << ... << std::forward<Args>(args));
        return oss.str();
    }
};
