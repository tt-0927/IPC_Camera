/**
 * @FilePath     : m3u8.h
 * @Author       : zhangjc (zhangjc@kfb.cn)
 * @Date         : 2024-11-04
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-06-30 20:30:51
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


class M3U8
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
    

    M3U8();
    M3U8(std::string path);
    ~M3U8();

    bool is_exit();
    /**
     * @brief CM3U8文件生成
     * @return int 
     */
    int create();
    
    int add_ts(Data_S stData);
    /**
     * @brief CM3U8文件生成
     * @return int 
     */
    int write_head();
    int write_data(Data_S stData);
    int write_tail();

    /**
     * @brief CM3U8文件解析
     * @return int 
     */
    int parse();
    /**
     * @brief CM3U8文件解析时间段
     * @param timeStr 
     * @return int 
     */
    int parse_time(const std::string &timeStr);

    /**
     * @brief 解析M3U8文件并提取ts文件名
     * @param strFilePath m3u8文件路径
     * @return vector ts文件名 
     */
    std::vector<std::string> get_M3u8TsFileName(const std::string& strFilePath);

    std::vector<Record_NS::VideoTime_S> get_videoTime();
private:
    /* 路径 */
    std::string m_path;
    bool m_isExit = false;
    std::ofstream m_outFile;
    std::vector<std::string> m_segmentTimes;
    std::vector<double> m_segmentDurations;
    std::vector<Record_NS::VideoTime_S> m_videoTimes;
};
