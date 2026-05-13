/**
 * @FilePath     : m3u8_file.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-28 10:36:11
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-07-08 17:27:54
 * @Description  : M3U8文件解析/生成
 */

#pragma once

#include <string>
#include <fstream>
#include <ostream>
#include <sstream>
#include <vector>
#include "record_define.h"

#ifdef __unix__
#include <unistd.h>
#endif

#include "type_define.h"
/*
文件内容格式

文件头
#EXTM3U
#EXT-X-VERSION:3
#EXT-X-TARGETDURATION:6
#EXT-X-MEDIA-SEQUENCE:0

ts文件信息
#EXT-X-PROGRAM-DATE-TIME:2025-01-13 08:40:58
#EXTINF:5.957000,
20250113_084058.ts
#EXT-X-PROGRAM-DATE-TIME:2025-01-13 15:36:18
#EXTINF:5.943000,
20250113_153618.ts

ts文件不连续相关时间
#END-TIME:1736753784738

#EXT-X-PROGRAM-DATE-TIME:2025-01-13 15:36:24
#EXTINF:112.451000,
20250113_153817.ts
#EXT-X-PROGRAM-DATE-TIME:2025-01-13 15:59:31
#EXTINF:5.932000,
20250113_155931.ts

ts文件不连续相关时间
#END-TIME:1736755177563

文件尾
#EXT-X-ENDLIST

*/
class CM3U8File
{
public:
    typedef struct
    {
        /* 开始时间 */
        std::string startTime;
        /* 时长 */
        int nDuration;
        /* 文件名 */
        std::string filename;
    } Data_S;

    CM3U8File() = default;
    CM3U8File(std::string path);
    ~CM3U8File();
    /**
     * @brief 设置M3U8文件路径
     * @param path 文件路径字符串
     * @return int 设置结果，0表示成功，非0表示失败
     */
    int set_path(std::string path);

    /**
     * @brief M3U8文件生成
     * @param path 生成的M3U8文件路径
     * @return int 生成结果，0表示成功，非0表示失败
     */
    int create(std::string &path);

    /**
     * @brief 添加TS片段信息到M3U8文件
     * @param m_stSliceInfo TS片段信息结构体
     * @return int 添加结果，0表示成功，非0表示失败
     */
    int add_ts(SliceInfo_S m_stSliceInfo);

    /**
     * @brief 写入M3U8文件头部信息
     * @return int 写入结果，0表示成功，非0表示失败
     */
    int write_head();

    /**
     * @brief 写入M3U8文件数据部分
     * @param stData 数据信息结构体
     * @return int 写入结果，0表示成功，非0表示失败
     */
    int write_data(Data_S stData);

    /**
     * @brief 写入M3U8文件尾部信息
     * @return int 写入结果，0表示成功，非0表示失败
     */
    int write_tail();

    /**
     * @brief 向文件添加空的时间段条目
     * @param file 文件流对象
     * @param nStartTime 开始时间
     * @param nEndTime 结束时间
     */
    void add_nullFile(std::fstream &file, int64_t nStartTime, int64_t nEndTime);

private:
    /* 路径 */
    std::string m_path;
    /*输出文件操作句柄*/
    std::ofstream m_outFile;
};
