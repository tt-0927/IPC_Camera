/**
 * @FilePath     : find_record_file.h
 * @Author       : zjc
 * @Date         : 2025-04-22 09:32:33
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-06-12 10:10:54
 * @Description  : 查找录像文件
 */

#pragma once

#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <deque>
#include <cstring>

class FindRecordFile 
{
public:
    FindRecordFile() = default;
    ~FindRecordFile() = default;
    time_t parseTime(const char* timeStr, const char* format);
    std::deque<std::string> find(int nChnId, std::string strStartTime, std::string strEndTime);

    /**
     * @brief 找到对应文件夹 根据时间
     * @param vecDir  
     * @param nStartDate  
     * @param nEndDate 
     * @return std::vector<std::string> 
     */
    std::vector<std::string> findDirByDate(const std::vector<std::string> &vecDir, int nStartDate, int nEndDate);
    /**
     * @brief 找到对应文件 根据时间
     * @param vecDir 
     * @param nStartDateTime 
     * @param nEndDateTime 
     * @return std::deque<std::string> 
     */
    std::deque<std::string>  findFileByTime(const std::vector<std::string> &vecDir, time_t nStartDateTime, time_t nEndDateTime);

};