#include "CheckFileThread.h"
#include "MaintenanceData.h"

#include <iostream>

#include <fstream>
#include <istream>
#include <ostream>
#include <string>
#include <sstream>
#include <regex>

#include <algorithm>

#include "dlog.h"

#ifdef WIN32
#include <io.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#endif

#define ERROR_START_TIME "1970"

using namespace MaintenanceNS;

CCheckFileThread::CCheckFileThread() : CMaintenanceThread()
{
}

void CCheckFileThread::init()
{
    if (m_bIsInit)
    {
        return;
    }

    MaintenanceManagerConf stConfig = CMaintenanceData::getInstance()->getConfig();
    if (stConfig.vecPaths.size() <= 0 ||
        stConfig.vecUploadFile.size() <= 0 ||
        stConfig.strDeviceCode.empty())
    {
        dlog_error("CCheckFileThread::init(), file path or file regex is empty! "
                  "Please initialize the configuration!");
        return;
    }
    m_vecPaths = stConfig.vecPaths;
    m_vecRegex = stConfig.vecUploadFile;
    m_strCurDate = CMaintenanceData::getInstance()->getCurDate();
    do
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        m_strCurDate = CMaintenanceData::getInstance()->getCurDate();
    } while (m_strCurDate.find(ERROR_START_TIME) != std::string::npos);

    m_strDeviceCode = stConfig.strDeviceCode;
    m_strRecordFilePath = stConfig.strRecordFilePath + "mainteanance_record/";
    m_strRecordFileFullPath = m_strRecordFilePath + getRecordFileName();
    m_strFilterFileFullPath = m_strRecordFilePath + "filter.json";

    CMaintenanceData::getInstance()->setRecordFileFullPath(m_strRecordFileFullPath);
    CMaintenanceData::getInstance()->setFilterFileFullPath(m_strFilterFileFullPath);

    m_bIsReadFilterFileFlag.store(true);
    m_bIsReadRecordFileFlag.store(true);
    m_bIsInit = true;
}

bool CCheckFileThread::isInit()
{
    return m_bIsInit;
}

void CCheckFileThread::run()
{
    int nIndex = -1;
    while (m_bIsRunFlag.load())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(CHECKFILE_MSLEEP));

        /* 读取过滤文件和读取本地记录文件，减少锁的颗粒度 */
        {
            /* 判断是日期是否和上次的一致 */
            std::string strTmpDate = CMaintenanceData::getInstance()->getCurDate();
            if (m_strCurDate.compare(strTmpDate) != 0)
            {
                dlog_info("old date:%s new date:%s", m_strCurDate.c_str(), strTmpDate.c_str());

                m_bIsReadFilterFileFlag.store(true);
                m_bIsReadRecordFileFlag.store(true);

                CMaintenanceData::getInstance()->nextDate();

                /* 更新读取的文件 */
                m_strCurDate = strTmpDate;
                m_strRecordFileFullPath = m_strRecordFilePath + getRecordFileName();
                CMaintenanceData::getInstance()->setRecordFileFullPath(m_strRecordFileFullPath);
            }

            /*读取过滤文件，用于过滤那天已经上传*/
            if (m_bIsReadFilterFileFlag.load())
            {
                m_vecFilterDate = CMaintenanceData::getInstance()->getFilterInfos(true);
                m_bIsReadFilterFileFlag.store(false);
            }

            /*读取本地记录文件*/
            if (m_bIsReadRecordFileFlag.load())
            {
                std::vector<RecordInfo> vecTmp = CMaintenanceData::getInstance()->getRecordInfos(true);
                if (vecTmp.size() > 0)
                {
                    m_bIsReadRecordFileFlag.store(false);
                }
            }
        }

        /* 判断是不是已经是下一天了，但是还没有上传完的情况 */
        if (CMaintenanceData::getInstance()->getNextDateStatus())
        {
            dlog_info("next date, wait uploaded...");
            /* 打印数据 */
            CMaintenanceData::getInstance()->printfAllContainers();
            /* 还没上传完，不遍历 */
            continue;
        }

        /* 打印数据 */
        /* CMaintenanceData::getInstance()->printfAllContainers(); */

        if (m_vecPaths.size() > 0)
        {
            if (nIndex < 0)
            {
                nIndex = 0;
            }
            else if (nIndex < m_vecPaths.size() - 1)
            {
                nIndex++;
            }
            else
            {
                nIndex = 0;
            }
        }
        else
        {
            nIndex = -1;
        }

        if (nIndex != -1)
        {
            iterateDir(m_vecPaths.at(nIndex));
        }
    }
}

std::string CCheckFileThread::getRecordFileName()
{
    /* 组装记录文件的文件名称 */
    std::stringstream strStream;
    strStream << "record_file_" << m_strCurDate << ".json";
    return strStream.str();
}

void CCheckFileThread::iterateDir(const std::string &strFilePath)
{
    std::string strTmp;
    int enFileType = FILE_TYPE_NORMAL;
    std::string strIdentifier;
    std::string strFileName;
    std::string strTmpDate;

#ifdef WIN32
    strTmp = strFilePath.substr(0, strFilePath.length() - 1) + "\\*";

    /* 文件句柄 */
    long hFile = 0;
    /* 文件信息结构体 */
    struct _finddata_t stFileinfo;
    /* 查找 */
    if ((hFile = _findfirst(strTmp.c_str(), &stFileinfo)) != -1)
    {
        /* 开始遍历 */
        do
        {
            /* 如果是文件 */
            if ((stFileinfo.attrib == _A_ARCH))
            {
                enFileType = FILE_TYPE_NORMAL;
                strTmpDate.clear();

                /*是否是需要的文件*/
                strFileName = stFileinfo.name;
                strIdentifier = m_strDeviceCode + "_" + strFileName;
                if (matchFile(strFileName, strTmpDate, enFileType))
                {
                    FileInfo stInfo;
                    stInfo.enFileType = static_cast<FileType>(enFileType);
                    stInfo.strIdentifier = strIdentifier;
                    stInfo.strFilePath = strFilePath;
                    stInfo.strFileName = strFileName;
                    stInfo.nFileSize = stFileinfo.size;
                    stInfo.strFileDate = strTmpDate;
                    /* 添加 */
                    CMaintenanceData::getInstance()->setFileInfoCheckRecord(stInfo);
                }
            }
        } while (_findnext(hFile, &stFileinfo) == 0);

        /* 关闭文件句柄 */
        _findclose(hFile);
    }
#else
    strTmp = strFilePath;
    DIR *pDir = opendir(strTmp.c_str());
    if (pDir == NULL)
    {
        dlog_warn("%s is not a directory or not exist!", strTmp.c_str());
        return;
    }

    std::string strFullFilePath;

    struct dirent *pDirent = NULL;
    while ((pDirent = readdir(pDir)) != NULL)
    {
        if (pDirent->d_type == DT_REG)
        {
            enFileType = FILE_TYPE_NORMAL;
            strTmpDate.clear();

            /*是否是需要的文件*/
            strFileName = pDirent->d_name;
            strIdentifier = m_strDeviceCode + "_" + strFileName;
            strFullFilePath = strFilePath + strFileName;

            /* TODO：如果文件名称是ApiToken.txt，用于测试Token失效的情况 */
            if (strFileName.find("ApiToken") != std::string::npos)
            {
                CMaintenanceData::getInstance()->setApiTokenFile(strFullFilePath);
            }

            if (matchFile(strFileName, strTmpDate, enFileType))
            {
                /* 获取文件大小 */
                struct stat stStat;
                int nRet = stat(strFullFilePath.c_str(), &stStat);
                if (nRet == 0)
                {
                    FileInfo stInfo;
                    stInfo.enFileType = static_cast<FileType>(enFileType);
                    stInfo.strIdentifier = strIdentifier;
                    stInfo.strFilePath = strFilePath;
                    stInfo.strFileName = strFileName;
                    stInfo.nFileSize = stStat.st_size;
                    stInfo.strFileDate = strTmpDate;
                    /* 添加 */
                    CMaintenanceData::getInstance()->setFileInfoCheckRecord(stInfo);
                }
                else
                {
                    dlog_error("read file size fail! file path: %s", strFullFilePath.c_str());
                }
            }
        }
    }
    closedir(pDir);
#endif
}

bool CCheckFileThread::matchFile(std::string strFileName, std::string &strDate, int &fileType)
{
    bool bMatch = false;
    /* 如果文件名称中存在着过去已经记录过的日期，返回false */
    for (std::size_t i = 0; i < m_vecFilterDate.size(); i++)
    {
        std::string strFilterDate = m_vecFilterDate.at(i);
        if (!strFilterDate.empty())
        {
            std::string strFilterDateOtherSplitter = strFilterDate;
            if (strFilterDate.find("-") != std::string::npos)
            {
                std::replace(strFilterDateOtherSplitter.begin(), strFilterDateOtherSplitter.end(), '-', '_');
            }
            else if (strFilterDate.find("_") != std::string::npos)
            {
                std::replace(strFilterDateOtherSplitter.begin(), strFilterDateOtherSplitter.end(), '_', '-');
            }

            if (strFileName.find(strFilterDate) != std::string::npos ||
                strFileName.find(strFilterDateOtherSplitter) != std::string::npos)
            {
                return bMatch;
            }
        }
    }

    /* 检查是否是当日的文件，是当日的文件return false */
    std::string strCurDateByOtherSplitter = m_strCurDate;
    if (m_strCurDate.find("-") != std::string::npos)
    {
        std::replace(strCurDateByOtherSplitter.begin(), strCurDateByOtherSplitter.end(), '-', '_');
    }
    else if (m_strCurDate.find("_") != std::string::npos)
    {
        std::replace(strCurDateByOtherSplitter.begin(), strCurDateByOtherSplitter.end(), '_', '-');
    }

    if (strFileName.find(m_strCurDate) != std::string::npos ||
        strFileName.find(strCurDateByOtherSplitter) != std::string::npos)
    {
        return bMatch;
    }

    /* 检查当前文件是否是需要上传的日志或者配置文件 */
    for (std::size_t i = 0; i < m_vecRegex.size(); i++)
    {
        std::regex cRegex(m_vecRegex.at(i).strFileNameFormat);
        bMatch = std::regex_search(strFileName, cRegex);
        if (bMatch)
        {
            fileType = static_cast<int>(m_vecRegex.at(i).enFileType);
            if (fileType == FILE_TYPE_LOG)
            {
                /* 如果文件名称中找不到日期，则不匹配 */
                bMatch = false;
                std::regex cRegexDate("\\d{4}-\\d{2}-\\d{2}|\\d{4}_\\d{2}_\\d{2}");
                std::match_results<std::string::const_iterator> stResults;
                if (std::regex_search(strFileName, stResults, cRegexDate))
                {
                    if (stResults.size() > 0)
                    {
                        bMatch = true;
                        std::ssub_match sub = stResults[0];
                        strDate = sub.str();
                    }
                }
            }
            break;
        }
    }
    return bMatch;
}
