#include "MaintenanceData.h"
#include <iostream>
#include <fstream>
#include <string>
#include <stdint.h>
#include <string.h>

#include <chrono>
#include <ctime>
#include <iomanip> // std::put_time

#include "dlog.h"

extern "C"
{
#include "share_define.h"
#include "edukit_network.h"
}

#ifdef WIN32
#include <io.h>
#include <direct.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#endif

using namespace MaintenanceNS;

CMaintenanceData *CMaintenanceData::m_pThis = nullptr;
std::mutex CMaintenanceData::m_thisMutex;

CMaintenanceData::CMaintenanceData()
{
}

CMaintenanceData *CMaintenanceData::getInstance()
{
    if (m_pThis == nullptr)
    {
        std::unique_lock<std::mutex> lock(m_thisMutex); // 加锁
        if (m_pThis == nullptr)
        {
            m_pThis = new (std::nothrow) CMaintenanceData();
        }
    }

    return m_pThis;
}

bool CMaintenanceData::init(const std::string &strConfigPath)
{
    std::unique_lock<std::shared_mutex> locker(m_configMutex);
    m_strConfigFilePath = strConfigPath;
    m_bIsInit = parseConfigFile();
    if (m_bIsInit)
    {
        // m_bIsInit = parseDeviceCode();
    }
    return m_bIsInit;
}

bool CMaintenanceData::init(std::string &strConfigureJson)
{
    std::unique_lock<std::shared_mutex> locker(m_configMutex);
    m_bIsInit = parseConfigByJson(strConfigureJson);
    return m_bIsInit;
}

void CMaintenanceData::init_config()
{
    m_stConfig.strCode = std::to_string(MQTT_PROJECT_ID);
    m_stConfig.strDeviceCode = MQTT_CODE;
    dlog_debug("CMaintenanceData配置初始化 code: %s device code: %s", m_stConfig.strCode.c_str(), m_stConfig.strDeviceCode.c_str());
    m_stConfig.vecPaths.push_back("/opt/course/log/");
    UploadConfig stUploadConfig;
    stUploadConfig.enFileType = FILE_TYPE_LOG;
    stUploadConfig.strFileNameFormat = "%Y-%m-%d";
    m_stConfig.vecUploadFile.push_back(stUploadConfig);
    m_bIsInit = true;
}

bool CMaintenanceData::isInit()
{
    return m_bIsInit;
}

MaintenanceManagerConf CMaintenanceData::getConfig()
{
    std::shared_lock<std::shared_mutex> locker(m_configMutex);
    return m_stConfig;
}

std::string CMaintenanceData::getRequeryUrl()
{
    std::shared_lock<std::shared_mutex> locker(m_configMutex);
    return m_stConfig.strUrl;
}

void CMaintenanceData::setDeviceCode(std::string strDeviceCode)
{
    std::unique_lock<std::shared_mutex> locker(m_configMutex);
    m_stConfig.strDeviceCode = strDeviceCode;
}

std::string CMaintenanceData::getDeviceCode()
{
    std::shared_lock<std::shared_mutex> locker(m_configMutex);
    return m_stConfig.strDeviceCode;
}

void CMaintenanceData::setProjectCode(std::string strProjectCode)
{
    std::unique_lock<std::shared_mutex> locker(m_configMutex);
    m_stConfig.strCode = strProjectCode;
}

std::string CMaintenanceData::getProjectCode()
{
    std::shared_lock<std::shared_mutex> locker(m_configMutex);
    return m_stConfig.strCode;
}

std::vector<std::string> CMaintenanceData::getFilesPath()
{
    std::shared_lock<std::shared_mutex> locker(m_configMutex);
    return m_stConfig.vecPaths;
}

std::string CMaintenanceData::getRecordFilePath()
{
    std::shared_lock<std::shared_mutex> locker(m_configMutex);
    return m_stConfig.strRecordFilePath;
}

std::string CMaintenanceData::getCurDate()
{
    return getStringDate("%Y-%m-%d");
}

std::string CMaintenanceData::getCurDateBySeparatorIs_()
{
    return getStringDate("%Y_%m_%d");
}

void CMaintenanceData::setLoginStatus(bool bStatus)
{
    std::unique_lock<std::shared_mutex> locker(m_loginMutex);
    m_bIsLogin = bStatus;
}

bool CMaintenanceData::getLoginStatus()
{
    std::shared_lock<std::shared_mutex> locker(m_loginMutex);
    return m_bIsLogin;
}

void CMaintenanceData::setToken(const std::string &strToken)
{
    std::unique_lock<std::shared_mutex> locker(m_tokenMutex);
    m_strToken = strToken;
}

void CMaintenanceData::setApiTokenFile(const std::string &strFilePath)
{
    std::unique_lock<std::shared_mutex> locker(m_tokenMutex);
    std::string strFile = strFilePath;
    /* 从文件中读取Token并覆盖 */
    std::string strTokenTmp = readFile(strFile);
    if (!strTokenTmp.empty())
    {
        char *pBufferTmp = new char[1];
        memset(pBufferTmp, 0, 1);

        wirteFile(strFile, pBufferTmp, 0);

        delete[] pBufferTmp;
        pBufferTmp = nullptr;

        m_strToken = strTokenTmp;
    }
}

std::string CMaintenanceData::getToken()
{
    std::shared_lock<std::shared_mutex> locker(m_tokenMutex);
    return m_strToken;
}

void CMaintenanceData::setProjectID(const int &nID)
{
    std::unique_lock<std::shared_mutex> locker(m_projectIDMutex);
    m_nProjectID = nID;
}

int CMaintenanceData::getProjectID()
{
    std::shared_lock<std::shared_mutex> locker(m_projectIDMutex);
    return m_nProjectID;
}

void CMaintenanceData::nextDate()
{
    m_bNextDate = true;
    /* 检查map是否已经全部上传完毕 */
    bool bIsAllUpload = true;
    std::set<std::string> setDates(m_vecFilter.begin(), m_vecFilter.end());

    /* 将历史记录里面记录的 */
    {
        std::shared_lock<std::shared_mutex> locker(m_recordMutex);
        for (std::size_t i = 0; i < m_vecRecord.size(); i++)
        {
            setDates.insert(m_vecRecord.at(i).stFileInfo.strFileDate);
        }
    }

    /* 检查map */
    {
        std::shared_lock<std::shared_mutex> locker(m_uploadRecordMutex);
        std::map<std::string, RecordInfo>::iterator ite = m_mapUploadRecord.begin();
        for (; ite != m_mapUploadRecord.end(); ite++)
        {
            RecordInfo stInfo = ite->second;
            dlog_info("record:%s status:%d date:%s", stInfo.stFileInfo.strIdentifier.c_str(), stInfo.enUploadStatus, stInfo.stFileInfo.strFileDate.c_str());
            if (stInfo.enUploadStatus != UPLOADED)
            {
                bIsAllUpload = false;
                break;
            }
            setDates.insert(stInfo.stFileInfo.strFileDate);
        }
    }

    if (bIsAllUpload)
    {
        m_strRecordJson.clear();

        {
            std::unique_lock<std::shared_mutex> locker(m_uploadRecordMutex);
            m_mapUploadRecord.clear();
        }
        /* 已经全部上传完成了，写出 */
        writeFilterFile(setDates);
        m_bNextDate = false;
    }
    /* 如果还没有上传完成，等待上传完成，修改最后的记录后，写出Filter
       changedRecordUploadStatus函数中进行判断
     */
}

bool CMaintenanceData::getNextDateStatus()
{
    return m_bNextDate;
}

void CMaintenanceData::changedRecordUploadStatus(const std::string &strIdentifier, const UploadStatus &enStatus)
{
    bool isAllUpload = true;
    {
        std::unique_lock<std::shared_mutex> locker(m_uploadRecordMutex);
        std::map<std::string, RecordInfo>::iterator ite = m_mapUploadRecord.begin();
        for (; ite != m_mapUploadRecord.end(); ite++)
        {
            RecordInfo stInfo = ite->second;
            if (stInfo.stFileInfo.strIdentifier.compare(strIdentifier) == 0)
            {
                stInfo.enUploadStatus = enStatus;
                ite->second.enUploadStatus = enStatus;

                /* 上传成功 */
                if (enStatus == UPLOADED)
                {
                    /* 添加进入记录列表 */
                    {
                        std::unique_lock<std::shared_mutex> locker(m_recordMutex);
                        m_vecRecord.push_back(stInfo);
                    }
                    /* 写出记录至记录文件 */
                    writeRecordFile(stInfo);
                }
                if (!m_bNextDate)
                {
                    break;
                }
            }
            else
            {
                if (stInfo.enUploadStatus != UPLOADED)
                {
                    isAllUpload = false;
                }
            }
        }
    }
    /* 如果已经是下一天了，并且已经上传完毕 */
    if (m_bNextDate && isAllUpload)
    {
        nextDate();
        if (!m_strRecordFilePathTmp.empty())
        {
            m_strRecordFilePath = m_strRecordFilePathTmp;
            m_strRecordFilePathTmp.clear();
        }
    }
}

void CMaintenanceData::printfAllContainers()
{
    //   dlog_debug( "\nv--------------CMaintenanceData::printfAllContainers--------------v");
    //   dlog_debug( "---start printf filter infos!");
    /* 打印过滤文件信息 */
    {
        std::shared_lock<std::shared_mutex> locker(m_filterMutex);
        for (std::size_t i = 0; i < m_vecFilter.size(); i++)
        {
            dlog_debug("No.%ld:%s", i + 1, m_vecFilter.at(i).c_str());
        }
    }

    dlog_debug("---start printf record infos!");
    /* 打印历史记录信息 */
    {
        std::shared_lock<std::shared_mutex> locker(m_recordMutex);
        for (std::size_t i = 0; i < m_vecRecord.size(); i++)
        {
            RecordInfo stInfo = m_vecRecord.at(i);
            dlog_debug("No.%ld:\n    file:%s/%s\n    upload status:%d",
                      i + 1,
                      stInfo.stFileInfo.strFilePath.c_str(),
                      stInfo.stFileInfo.strFileName.c_str(),
                      stInfo.enUploadStatus);
        }
    }

    dlog_debug("---start printf map record infos!");
    /* 打印map记录信息 */
    {
        std::shared_lock<std::shared_mutex> locker(m_uploadRecordMutex);
        int i = 0;
        std::map<std::string, RecordInfo>::iterator ite = m_mapUploadRecord.begin();
        for (; ite != m_mapUploadRecord.end(); ite++)
        {
            RecordInfo stInfo = ite->second;
            dlog_debug("No.%d:\n    file:%s/%s\n    upload status:%d",
                      i + 1,
                      stInfo.stFileInfo.strFilePath.c_str(),
                      stInfo.stFileInfo.strFileName.c_str(),
                      stInfo.enUploadStatus);
            i++;
        }
    }
    //   dlog_debug( "^--------------CMaintenanceData::printfAllContainers--------------^\n");
}

void CMaintenanceData::setFileInfoCheckRecord(FileInfo &stFileInfo)
{
    /* 检查是否存在map中 */
    {
        std::shared_lock<std::shared_mutex> locker(m_uploadRecordMutex);
        if (m_mapUploadRecord.find(stFileInfo.strIdentifier) !=
            m_mapUploadRecord.end())
        {
            return;
        }
    }
    /* 检查是否存在历史记录中 */
    {
        std::shared_lock<std::shared_mutex> locker(m_recordMutex);
        for (std::size_t i = 0; i < m_vecRecord.size(); i++)
        {
            if (m_vecRecord.at(i).stFileInfo.strIdentifier.compare(stFileInfo.strIdentifier) == 0)
            {
                return;
            }
        }
    }
    /* 都不存在，那么加入到map中 */
    std::unique_lock<std::shared_mutex> locker(m_uploadRecordMutex);
    RecordInfo stRecordInfo;
    stRecordInfo.stFileInfo = stFileInfo;
    stRecordInfo.enUploadStatus = UPLOAD_NOT;
    m_mapUploadRecord.insert({stFileInfo.strIdentifier, stRecordInfo});
}

RecordInfo CMaintenanceData::getNeedUploadFile()
{
    std::unique_lock<std::shared_mutex> locker(m_uploadRecordMutex);
    std::map<std::string, RecordInfo>::iterator ite = m_mapUploadRecord.begin();
    for (; ite != m_mapUploadRecord.end(); ite++)
    {
        RecordInfo stInfo = ite->second;
        if (stInfo.enUploadStatus == UPLOAD_NOT)
        {
            return stInfo;
        }
    }
    return RecordInfo();
}

std::string CMaintenanceData::getStringDate(const char *strFormat)
{
    /*获取当前系统时间的时间戳*/
    auto currentTimeStamp = std::chrono::system_clock::now();
    /*转换为当前时区的时间*/
    std::time_t currentTime = std::chrono::system_clock::to_time_t(currentTimeStamp);
    /*将时间转换为struct tm结构体*/
    struct std::tm *currentTimeInfo = std::localtime(&currentTime);
    /*输出格式化的日期时间*/
    std::stringstream strStream;
    strStream << std::put_time(currentTimeInfo, strFormat);
    return strStream.str();
}

bool CMaintenanceData::createDirectory(const std::string &strPath)
{
    uint32_t dirPathLen = strPath.length();
    if (dirPathLen > MAX_PATH_LEN)
    {
        return -1;
    }
    char tmpDirPath[MAX_PATH_LEN] = {0};
    for (uint32_t i = 0; i < dirPathLen; ++i)
    {
        tmpDirPath[i] = strPath[i];
        if (tmpDirPath[i] == '\\' || tmpDirPath[i] == '/')
        {
            if (ACCESS(tmpDirPath, 0) != 0)
            {
                int32_t ret = MKDIR(tmpDirPath);
                if (ret != 0)
                {
                    return ret;
                }
            }
        }
    }
    return 0;
}

void CMaintenanceData::setRecordFileFullPath(const std::string &strFileFullPath)
{
    if (!m_bNextDate)
    {
        m_strRecordFilePath = strFileFullPath;
    }
    else
    {
        m_strRecordFilePathTmp = strFileFullPath;
    }
}

void CMaintenanceData::setFilterFileFullPath(const std::string &strFileFullPath)
{
    m_strFilterFilePath = strFileFullPath;
}

std::vector<RecordInfo> CMaintenanceData::getRecordInfos(bool bReParse)
{
    if (bReParse)
    {
        std::unique_lock<std::shared_mutex> locker(m_recordMutex);
        parseRecordFile();
        return m_vecRecord;
    }
    else
    {
        std::shared_lock<std::shared_mutex> locker(m_recordMutex);
        return m_vecRecord;
    }
}

std::vector<std::string> CMaintenanceData::getFilterInfos(bool bReParse)
{
    if (bReParse)
    {
        std::unique_lock<std::shared_mutex> locker(m_filterMutex);
        parseFilterFile();
        return m_vecFilter;
    }
    else
    {
        std::shared_lock<std::shared_mutex> locker(m_filterMutex);
        return m_vecFilter;
    }
}

std::string CMaintenanceData::readFile(std::string &strFile)
{
    std::string strContext;

    /* 如果文件夹不存在，则创建 */
    createDirectory(strFile);

    /* 读写，以追加形式打开文件，文件不存在会创建 */
    std::fstream inStream(strFile.c_str(), std::ios::in | std::ios::out | std::ios::app);
    if (inStream.is_open())
    {
        std::string strTmp;
        while (!inStream.eof())
        {
            inStream >> strTmp;
            strContext.append(strTmp);
        }
        inStream.close();
    }
    else
    {
        dlog_error("file path error! path:%s", strFile.c_str());
    }
    return strContext;
}

bool CMaintenanceData::parseConfigFile()
{
    std::string strJson = readFile(m_strConfigFilePath);

    if (strJson.length() > 0)
    {
        m_stConfig = m_cParse.parseConfig(strJson);
        if (!m_stConfig.strUrl.empty() &&
            !m_stConfig.strCode.empty() && m_stConfig.vecUploadFile.size() > 0)
        {
            return true;
        }
    }
    dlog_error("init config fial!");
    return false;
}

bool CMaintenanceData::parseConfigByJson(std::string strConfigureJson)
{
    if (strConfigureJson.length() > 0)
    {
        m_stConfig = m_cParse.parseConfig(strConfigureJson);
        if (!m_stConfig.strUrl.empty() && !m_stConfig.strDeviceCode.empty() &&
            !m_stConfig.strRecordFilePath.empty() &&
            !m_stConfig.strCode.empty() && m_stConfig.vecUploadFile.size() > 0)
        {
            return true;
        }
    }
    dlog_error("init config fial!");
    return false;
}

bool CMaintenanceData::parseRecordFile()
{
    std::string strJson = readFile(m_strRecordFilePath);

    if (strJson.length() > 0)
    {
        m_strRecordJson = strJson;
        m_vecRecord.clear();

        m_vecRecord = m_cParse.parseRecordFileData(strJson);
        if (m_vecRecord.size() > 0)
        {
            return true;
        }
    }
    dlog_warn("file is empty or pase record file fial!");
    return false;
}

bool CMaintenanceData::parseFilterFile()
{
    std::string strJson = readFile(m_strFilterFilePath);

    if (strJson.length() > 0)
    {
        m_vecFilter.clear();

        m_vecFilter = m_cParse.parseFilterFileData(strJson);
        if (m_vecFilter.size() > 0)
        {
            return true;
        }
    }
    dlog_warn("file is empty or pase filter file fial!");
    return false;
}

bool CMaintenanceData::wirteFile(std::string &strFile, char *buffer, size_t size)
{
    /* 读写，以截断形式打开文件，文件不存在会创建 */
    std::fstream outStream(strFile.c_str(), std::ios::in | std::ios::out | std::ios::trunc);
    if (outStream.is_open())
    {
        outStream.write(buffer, size);
        outStream.close();
        return true;
    }
    else
    {
        dlog_error("write file path error! path:%s", strFile.c_str());
    }
    return false;
}

bool CMaintenanceData::writeRecordFile(const RecordInfo &stRecordInfo)
{
    std::size_t nSize = 0;
    char *pJsonBuffer = m_cParse.createRecordFileBuffer(m_strRecordJson, stRecordInfo, nSize);
    if (pJsonBuffer != nullptr && nSize > 0)
    {
        wirteFile(m_strRecordFilePath, pJsonBuffer, nSize);
        return true;
    }
    return false;
}

bool CMaintenanceData::writeFilterFile(std::set<std::string> &setFilter)
{
    std::vector<std::string> vecFilter(setFilter.begin(), setFilter.end());
    std::size_t nSize = 0;
    char *pJsonBuffer = m_cParse.createFilterFileBuffer(vecFilter, nSize);
    if (pJsonBuffer != nullptr && nSize > 0)
    {
        wirteFile(m_strFilterFilePath, pJsonBuffer, nSize);
        return true;
    }
    return true;
}
