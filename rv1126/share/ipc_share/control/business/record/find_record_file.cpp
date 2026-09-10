/**
 * @FilePath     : find_record_file.cpp
 * @Author       : zjc
 * @Date         : 2025-04-22 09:32:33
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-06-12 10:36:53
 * @Description  : 查找录像文件
 */

#include "find_record_file.h"

#include <iostream>
#include <chrono>
#include <memory>
#include <fstream>
#include <filesystem>
#include <ctime>

#include "dlog.h"
#include "action_code.h"
// #include "disk_manage.h"

namespace fs = std::filesystem;
// 支持自定义格式的时间字符串解析函数
time_t FindRecordFile::parseTime(const char *timeStr, const char *format)
{
    std::tm tm = {};
    if (!strptime(timeStr, format, &tm))
    {
        dlog_error("解析失败:%s", timeStr);
        return -1;
    }
    return mktime(&tm);
}

std::deque<std::string> FindRecordFile::find(int nChnId, std::string strStartTime, std::string strEndTime)
{
    dlog_info("\nChnId:%d strStartTime:%s strEndTime:%s",
             nChnId, strStartTime.c_str(), strEndTime.c_str());

    // time_t nStartDateTime = parseTime(strStartTime.c_str(), "%Y%m%d_%H%M%S");
    // time_t nEndDateTime = parseTime(strEndTime.c_str(), "%Y%m%d_%H%M%S");

    // std::string strStartDate;
    // std::string strEndDate;
    // std::size_t nDataIndex = strStartTime.find('_');
    // if (nDataIndex != std::string::npos && (nDataIndex + 1) < strStartTime.length())
    // {
    //     strStartDate = strStartTime.substr(0, nDataIndex);
    // }

    // nDataIndex = strEndTime.find('_');
    // if (nDataIndex != std::string::npos && (nDataIndex + 1) < strEndTime.length())
    // {
    //     strEndDate = strEndTime.substr(0, nDataIndex);
    // }

    // int nStartDate = std::atoi(strStartDate.c_str());
    // int nEndDate = std::atoi(strEndDate.c_str());

    // dlog_info("startDate:%d endDate:%d  nStartDateTime:%lld nEndDateTime:%lld", nStartDate, nEndDate, nStartDateTime, nEndDateTime);

    // std::vector<std::string> vecDiskPath;
    // std::vector<System::DiskInfo_S> vecDiskInfo = CDiskManage::instance()->get_disksInfo();
    // for (size_t i = 0; i < vecDiskInfo.size(); i++)
    // {
    //     std::string strPath = vecDiskInfo.at(i).strMountPath + "/record/D" + std::to_string(nChnId + 1);
    //     vecDiskPath.push_back(strPath);
    // }

    // std::vector<std::string> vecDir = findDirByDate(vecDiskPath, nStartDate, nEndDate);
    // std::deque<std::string> dequeFile = findFileByTime(vecDir, nStartDateTime, nEndDateTime);
    std::deque<std::string> dequeFile;
    return dequeFile;
}

std::vector<std::string> FindRecordFile::findDirByDate(
    const std::vector<std::string> &vecDir,
    int nStartDate,
    int nEndDate)
{
    std::vector<std::string> vec;

    for (int i = 0; i < vecDir.size(); i++)
    {
        fs::path directory = vecDir.at(i);

        dlog_info("查询目录：%s", vecDir.at(i).c_str());

        if (!fs::exists(directory) || !fs::is_directory(directory))
        {
            dlog_info("指定路径不存在或不是一个目录：%s", directory.c_str());
            continue;
        }

        for (const auto &entry : fs::recursive_directory_iterator(directory))
        {
            if (entry.is_directory())
            {
                dlog_info("    目录：%s", entry.path().c_str());

                std::string strPath = entry.path().string();
                std::string strDirName;
                std::size_t nIndex = std::string::npos;
                std::size_t nIndexTmp = strPath.find('/');

                while (nIndexTmp != std::string::npos)
                {
                    nIndex = nIndexTmp;
                    if (nIndexTmp + 1 < strPath.length())
                    {
                        nIndexTmp = strPath.find('/', nIndexTmp + 1);
                    }
                }

                if (nIndex != std::string::npos && nIndex + 1 < strPath.length())
                {
                    strDirName = strPath.substr(nIndex + 1);
                }

                if (!strDirName.empty())
                {
                    int nDate = std::atoi(strDirName.c_str());

                    if (nDate >= nStartDate && nDate <= nEndDate)
                    {
                        vec.push_back(strPath);
                    }
                }
            }
        }
    }

    return vec;
}

/* 自然排序 */
static bool naturalSort(const std::string &a, const std::string &b)
{
    size_t i = 0, j = 0;
    while (i < a.size() && j < b.size())
    {
        if (std::isdigit(a[i]) && std::isdigit(b[j]))
        {
            // 比较数字部分
            size_t num1 = 0, num2 = 0;
            while (i < a.size() && std::isdigit(a[i]))
            {
                num1 = num1 * 10 + (a[i] - '0');
                ++i;
            }
            while (j < b.size() && std::isdigit(b[j]))
            {
                num2 = num2 * 10 + (b[j] - '0');
                ++j;
            }
            if (num1 != num2)
                return num1 < num2;
        }
        else
        {
            // 比较非数字部分
            char ca = std::tolower(a[i]);
            char cb = std::tolower(b[j]);
            if (ca != cb)
                return ca < cb;
            ++i;
            ++j;
        }
    }
    return i == a.size() && j < b.size();
}

std::deque<std::string> FindRecordFile::findFileByTime(
    const std::vector<std::string> &vecDir,
    time_t nStartDateTime,
    time_t nEndDateTime)
{
    std::vector<std::string> vecFile;

    for (int i = 0; i < vecDir.size(); i++)
    {
        fs::path directory = vecDir.at(i);

        dlog_info("查询目录：%s", vecDir.at(i).c_str());

        if (!fs::exists(directory) || !fs::is_directory(directory))
        {
            dlog_info("指定路径不存在或不是一个目录：%s", directory.c_str());
            break;
        }

        for (const auto &entry : fs::recursive_directory_iterator(directory))
        {
            if (entry.is_regular_file())
            {
                std::string strFileName = entry.path().filename();
                std::size_t nIndex = strFileName.find(".ts");

                if (nIndex != std::string::npos)
                {
                    std::string strTimeTmp = strFileName.substr(0, nIndex);

                    if (!strTimeTmp.empty())
                    {
                        time_t nDateTime = parseTime(strTimeTmp.c_str(), "%Y%m%d_%H%M%S");

                        if (nDateTime > 0 && (nDateTime >= nStartDateTime && nDateTime <= nEndDateTime))
                        {
                            vecFile.push_back(entry.path().string());
                        }
                    }
                }
            }
        }
    }

    /* 按照文件名称自然排序 */
    // std::sort(vecFile.begin(), vecFile.end(), naturalSort);

    std::deque<std::string> dequeFile(vecFile.begin(), vecFile.end());

    return dequeFile;
}
