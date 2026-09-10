/**
 * @FilePath     : data_tools.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-28 10:36:11
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-07-01 19:50:28
 * @Description  : 数据转换接口函数接口
 */

#pragma once

#include <cstddef>
#include <iostream>
#include <string>

class CDataTools
{
public:
    /**
     * @brief 判断h264数据是否为关键帧
     * @param [char*] pchData: 数据
     * @param [int] nSize: 数据大小
     * @return [*] true-关键帧  false-非关键帧
     * @note
     */
    static bool isIFrame_h264(const char *pchData, int nSize);

    /**
     * @brief 判断h265数据是否为关键帧
     * @param [char*] pchData: 数据
     * @param [int] nSize: 数据大小
     * @return [*] true-关键帧  false-非关键帧
     * @note
     */
    static bool isIFrame_h265(const char *pchData, int nSize);

    /**
     * @brief 判断是否为h264数据
     * @param [char*] pchData: 数据
     * @param [int] nSize: 数据大小
     * @return [*] true-是 false-不是
     * @note
     */
    static bool is_h264Format(const char *pchData, int nSize);

    /**
     * @brief 判断是否为h265数据
     * @param [char*] pchData: 数据
     * @param [int] nSize: 数据大小
     * @return [*] true-是 false-不是
     * @note
     */
    static bool is_h265Format(const char *pchData, int nSize);

    /**
     * @brief 解析-SPS-h264数据
     * @param [char*] pchData: 数据
     * @param [int] nLen: 数据长度
     * @param [int*] pnWidth: 解析到的宽
     * @param [int*] pnHeight: 解析到的高
     * @param [int*] pnFps: 解析到的FPS
     * @return [*]
     * @note
     */
    static bool parse_h264SPS(char *pchData, int nLen, int *pnWidth, int *pnHeight, int *pnFps);

    /**
     * @brief 解析-SPS-h265数据
     * @param [char*] pchData: 数据
     * @param [int] nLen: 数据长度
     * @param [int*] pnWidth: 解析到的宽
     * @param [int*] pnHeight: 解析到的高
     * @param [int*] pnFps: 解析到的FPS
     * @return [*]
     * @note
     */
    static bool parse_h265SPS(char *pchData, int nLen, int *pnWidth, int *pnHeight, int *pnFps);

    /**
     * @brief 写入文件
     * @param [char*] pchFilePath: 文件路径
     * @param [char*] pchJsonData: 写入信息
     * @return [*] int::0 成功  其他失败
     * @note
     */
    static int write_toFile(const char *pchFilePath, const char *pchJsonData);

    /**
     * @brief 读取文件
     * @param [char*] pchFilePath: 文件路径
     * @return [*] NULL-失败 文件数据指针-常规
     * @note 需要调用者释放返回值空间 free();
     */
    static char *read_fromFile(const char *pchFilePath);

    /**
     * @brief 耗时打印
     * @param [char*] pText: 输出字符串
     * @return [*]
     * @note pText == NULL时，清空计时
     */
    static void PrintMs(const char *pText);

    /**
     * @brief 追加文件数据
     * @param [char*] pchFileName: 文件名
     * @param [void*] pData: 写入的数据
     * @param [size_t] nDataSize: 写入的数据大小
     * @return [*] 成功 0
     * @note
     */
    static int appendDataToFile(const char *pchFileName, const void *pData, size_t nDataSize);

    /**
     * @brief 打开文件并清空原有的数据，写入文件
     * @param [char*] pchFileName: 文件名
     * @param [void*] pData: 写入的数据
     * @param [size_t] nDataSize: 写入的数据大小
     * @return [*] 成功 0
     * @note
     */
    static int writeDataToFile(const char *pchFileName, const void *pData, size_t nDataSize);

    /**
     * @brief 删除空文件夹
     * @param [string&] strFolderPath: 文件夹路径
     * @return [*]
     * @note
     */
    static bool deleteNullFolder(const std::string &strFolderPath);

    /**
     * @brief 创建文件/文件夹
     * @param [string&] strFilePath: 文件路径
     * @return [*]
     * @note
     */
    static bool createFile(const std::string &strFilePath);
};
