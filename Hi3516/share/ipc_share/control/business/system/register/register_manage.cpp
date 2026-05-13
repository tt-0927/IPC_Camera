/**
 * @FilePath     : register_manage.cpp
 * @Author       : huangjunda
 * @Date         : 2026-01-21 11:12:16
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-03-31 11:29:59
 * @Description  : 注册管理
 */

#include "register_manage.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <sys/stat.h>

#include "md5c.h"
#include "cJSON.h"
#include "ini_disposed.h"
#include "data_length.h"
#include "time_utils.h"

CRegisterManage::CRegisterManage() : m_bStopFlag(false)
{
}

IpcRet_E CRegisterManage::init()
{
    /*确定目录是否存在，不存在则创建目录*/
    create_file_path(REGISTER_INFO_FILE);
    create_file_path(REGISTER_CODE_MANAGE_PATH);

    /* 初始化配置文件 */
    init_config();

    /* 必须等待一秒，不然初始化定时器会异常 */
    sleep(1);

    /* 初始化定时器 */
    init_timer();

    return OK;
}

IpcRet_E CRegisterManage::deinit()
{
    /* 停止线程并等待退出 */
    m_bStopFlag = true;
    // m_cv.notify_all();
    if (m_thread.joinable())
    {
        m_thread.join();
    }
    return OK;
}

/* 激活/注册设备 */
IpcRet_E CRegisterManage::register_device(std::string strRegisterCode)
{
    IpcRet_E enRetCode = OK;
    Register::RegisterInfo_S stInfo;

    /* 获取系统注册信息 */
    get_register_info(stInfo);

    /*已经永久激活就不需要在激活了*/
    if (stInfo.enActionTime == Register::ActivationTime_E::AT_FOREVER)
    {
        dlog_error("已经永久激活就不需要在激活了！");
        return ERR_REGISTER_FOREVER;
    }

    /* 检查激活/注册码是否被使用 */
    if (OK_EXIST == check_code_used(strRegisterCode))
    {
        dlog_error("该激活/注册码已被使用");
        return ERR_REGISTER_CODE_REUSE;
    }

    /* 激活 */
    char achTime[LENGTH64] = {0};

    /*解密-获取激活时间*/
    enRetCode = set_register(stInfo.strMachinSn, strRegisterCode, stInfo.enActionTime);
    if (enRetCode != OK)
    {
        dlog_error("激活/注册序列号：[%s] 失败!!", strRegisterCode.c_str());
        /* 激活/注册码错误 */
        return enRetCode;
    }
    dlog_info("激活/注册成功!!! 有效期限:[%d] 序列号[%s]", stInfo.enActionTime, strRegisterCode.c_str());

    /* 激活/注册成功，更新配置信息 */
    stInfo.strRegisterEg = strRegisterCode;

    /* 记录一下当前激活/注册成功的系统时间 */
    /* 获取当前日期 */
    struct tm *pTm = NULL;
    time_t timep;
    time(&timep);
    if (timep == (time_t)-1)
    {
        dlog_error("获取当前时间-失败");
        return ERR_REGISTER_FAULT;
    }

    pTm = localtime(&timep);
    if (pTm == NULL)
    {
        dlog_error("转换当前时间失败");
        return ERR_REGISTER_FAULT;
    }

    snprintf(achTime, LENGTH64, "%04d-%02d-%02d %02d:%02d:%02d", pTm->tm_year + 1900, pTm->tm_mon + 1, pTm->tm_mday, pTm->tm_hour, pTm->tm_min, pTm->tm_sec);
    stInfo.strStartTime = TimeUtils_NS::get_currentDateAndTimeNoT();
    dlog_info("记录一下当前激活/注册成功的时间:%s", stInfo.strStartTime.c_str());

    /* 记录当前检查时间 */
    stInfo.strLatestCheckTime = stInfo.strStartTime;

    switch (stInfo.enActionTime)
    {
    case Register::ActivationTime_E::AT_ONE_WEEK:
    {
        stInfo.lnLifeTimer = 7 * 24 * 60;
        break;
    }
    case Register::ActivationTime_E::AT_ONE_MONTH:
    {
        stInfo.lnLifeTimer = 30 * 24 * 60;
        break;
    }
    case Register::ActivationTime_E::AT_TWO_MONTH:
    {
        stInfo.lnLifeTimer = 60 * 24 * 60;
        break;
    }
    case Register::ActivationTime_E::AT_THREE_MONTH:
    {
        stInfo.lnLifeTimer = 90 * 24 * 60;
        break;
    }
    case Register::ActivationTime_E::AT_HALF_YEAR:
    {
        stInfo.lnLifeTimer = 180 * 24 * 60;
        break;
    }
    case Register::ActivationTime_E::AT_FOREVER:
    {
        stInfo.lnLifeTimer = PERMANENT_TIMER;
        break;
    }
    default:
    {
        break;
    }
    }

    dlog_info("有效时间:%lld", stInfo.lnLifeTimer);

    write_register_code(strRegisterCode);

    /* 更新激活/注册状态 */
    enRetCode = set_register_info(stInfo);
    if (enRetCode < OK)
    {
        dlog_error("更新激活/注册状态-失败");
        enRetCode = ERR_FREAD;
    }

    return enRetCode;
}

/* 获取激活/注册信息 */
IpcRet_E CRegisterManage::get_register_info(Register::RegisterInfo_S &strInfo)
{
    std::lock_guard<std::mutex> lock(m_registerMutex);
    strInfo = m_stRegister;
    return OK;
}

/* 初始化定时器 */
IpcRet_E CRegisterManage::init_timer()
{
#if !IS_RUNING_TIME_CHECK_MODE
    check_registerInfo(this);
#endif

    /* 采用定时器*/
    // AVTimer::instance().addTimer(m_nTimerId,
    //                              TIMEROUT_TIME * TIMEROUT_UNIT,
    //                              CRegisterManage::check_registerInfo, this);
    m_thread = std::thread(&CRegisterManage::timer_thread, this);
    m_thread.detach();

    return OK;
}

void CRegisterManage::timer_thread()
{
    pthread_setname_np(pthread_self(), "RegTimerthr");

    // std::unique_lock<std::mutex> lock(m_mutex);
    while (!m_bStopFlag.load())
    {
        // 等待1小时或收到停止信号
        std::this_thread::sleep_for(std::chrono::hours(1));
        // 执行注册信息检查
        check_registerInfo(this);

        // // 等待1小时或收到停止信号
        // m_cv.wait_for(lock, std::chrono::hours(1), [this]()
        //               { return m_bStopFlag.load(); });
    }
}

/* 初始化配置文件 */
IpcRet_E CRegisterManage::init_config()
{
    IpcRet_E enRetCode = OK;

    Register::RegisterInfo_S stInfo;

    enRetCode = load_register_info(stInfo);

    /* 设置注册信息 */
    enRetCode = set_register_info(stInfo);
    if (enRetCode != OK)
    {
        dlog_error("设置注册信息-失败");
    }
    else
    {
        dlog_trace("设置注册信息-成功");
    }

    /* 更新备份文件 */
    enRetCode = write_registerJson(stInfo, std::string(REGISTER_BACKUP_FILE_PATH));
    if (enRetCode != OK)
    {
        dlog_error("更新备份文件[%s]-失败", REGISTER_BACKUP_FILE_PATH);
    }
    else
    {
        dlog_trace("更新本地文件[%s]-成功", REGISTER_BACKUP_FILE_PATH);
    }

    return enRetCode;
}

IpcRet_E CRegisterManage::load_register_info(Register::RegisterInfo_S &stInfo)
{
    /* 主文件读取返回值 */
    IpcRet_E enRetCode = OK;

    /* 优先读取主注册文件，保持与原有数据源一致 */
    if (access(REGISTER_INFO_FILE, F_OK) == 0)
    {
        enRetCode = read_registerJson(stInfo, std::string(REGISTER_INFO_FILE));
        if (enRetCode == OK)
        {
            dlog_trace("读取本地文件[%s]-成功", REGISTER_INFO_FILE);
            return OK;
        }

        dlog_error("读取本地文件[%s]-失败", REGISTER_INFO_FILE);
        dlog_trace("尝试读取备份文件[%s]", REGISTER_BACKUP_FILE_PATH);
    }
    else
    {
        dlog_trace("本地文件不存在[%s]-尝试读取备份文件[%s]",
                   REGISTER_INFO_FILE,
                   REGISTER_BACKUP_FILE_PATH);
    }

    /* 主文件读取失败后，再尝试读取备份文件 */
    if (access(REGISTER_BACKUP_FILE_PATH, F_OK) != 0)
    {
        dlog_trace("本地文件和备份文件都不存在-重新创建");
        return OK_NOT_EXIST;
    }

    /* 备份文件中的注册信息 */
    Register::RegisterInfo_S stBackupInfo;
    /* 备份文件读取返回值 */
    IpcRet_E enBackupRet = read_registerJson(stBackupInfo, std::string(REGISTER_BACKUP_FILE_PATH));
    if (enBackupRet != OK)
    {
        dlog_error("读取本地备份文件[%s]-失败", REGISTER_BACKUP_FILE_PATH);
        return enBackupRet;
    }

    stInfo = stBackupInfo;
    dlog_trace("读取本地备份文件[%s]-成功", REGISTER_BACKUP_FILE_PATH);
    return OK;
}

/* 定时器超时-检查激活/注册信息 */
int CRegisterManage::check_registerInfo(void *pArgv)
{
    dlog_trace("定时检测注册信息");
    // char achTime[LENGTH64] = {0};
    CRegisterManage *pHandle = (CRegisterManage *)pArgv;
    if (pHandle)
    {
        Register::RegisterInfo_S stInfo;
        /* 配置文件读取失败时，继续使用缓存信息，避免异常文件覆盖当前注册状态 */
        if (pHandle->load_register_info(stInfo) != OK)
        {
            dlog_error("加载注册信息失败，使用缓存信息继续校验");
            pHandle->get_register_info(stInfo);
        }

        /* 已过期 */
        if (stInfo.lnLifeTimer <= 0 && stInfo.enActionTime != Register::ActivationTime_E::AT_NULL &&
            stInfo.enActionTime != Register::ActivationTime_E::AT_FOREVER)
        {
            stInfo.enActionTime = Register::ActivationTime_E::AT_EXPIRED;
            dlog_trace("注册码已过期");
        }

        if (stInfo.lnLifeTimer > 0 && stInfo.enActionTime != Register::ActivationTime_E::AT_NULL &&
            stInfo.enActionTime != Register::ActivationTime_E::AT_FOREVER &&
            stInfo.enActionTime != Register::ActivationTime_E::AT_EXPIRED)
        {
            /* 获取当前时间 */
            time_t nCurrentTime;
            time(&nCurrentTime);
            if (nCurrentTime == (time_t)-1)
            {
                dlog_error("获取当前时间-失败");
                return -1;
            }

            /*转换成结构体*/
            struct tm *pTm = NULL;
            pTm = localtime(&nCurrentTime);
            if (pTm == NULL)
            {
                dlog_error("转换获取的当前时间-失败");
                return -1;
            }

            /* 检查模式 */
#if IS_RUNING_TIME_CHECK_MODE
            if (stInfo.lnLifeTimer < TIMEROUT_TIME)
            {
                stInfo.lnLifeTimer = 0;
            }
            else
            {
                stInfo.lnLifeTimer -= TIMEROUT_TIME;
            }
            /* 重新设置上一次修改时间 */
            snprintf(achTime, LENGTH64, "%04d-%02d-%02d %02d:%02d:%02d", pTm->tm_year + 1900, pTm->tm_mon + 1, pTm->tm_mday, pTm->tm_hour, pTm->tm_min, pTm->tm_sec);
            stInfo.strLatestCheckTime = achTime;
#else
            /* 获取上一次检查的时间 */
            struct tm stLatestCheckTime;
            /*将"YYYY-MM-DD hh:mm:ss" 转换为tm*/
            if (NULL == strptime(stInfo.strLatestCheckTime.c_str(), "%Y-%m-%d %H:%M:%S", &stLatestCheckTime))
            {
                dlog_error("解析获取的上一次修改时间-失败");
                return -1;
            }
            /*将 tm 转换为1970年以来的秒*/
            time_t nLatestCheckTime = mktime(&stLatestCheckTime);
            if (nLatestCheckTime == -1)
            {
                dlog_error("转换获取的上一次修改时间-失败");
                return -1;
            }

            /*当前与上一次检查时间的差值*/
            char achTime[LENGTH64] = {0};
            long int nDiffs = static_cast<long int>(nCurrentTime - nLatestCheckTime);
            long int nMinute = nDiffs / 60;
            if (nMinute >= 0)
            {
                if (stInfo.lnLifeTimer < nMinute)
                {
                    stInfo.lnLifeTimer = 0;
                }
                else
                {
                    stInfo.lnLifeTimer -= nMinute;
                }

                dlog_trace("使用时长减少 [%ld]分钟", nMinute);

                /* 重新设置上一次修改时间 */
                snprintf(achTime, LENGTH64, "%04d-%02d-%02d %02d:%02d:%02d", pTm->tm_year + 1900, pTm->tm_mon + 1, pTm->tm_mday, pTm->tm_hour, pTm->tm_min, pTm->tm_sec);
                stInfo.strLatestCheckTime = achTime;
            }
#endif
        }
        pHandle->set_register_info(stInfo);
    }
    return 0;
}

/* 将Json数据转化为结构体信息 */
IpcRet_E CRegisterManage::conver_jsonToStruct(Register::RegisterInfo_S &stInfo, char *pJsonBuf)
{
    if (NULL == pJsonBuf)
    {
        dlog_error("传入参数异常");
        return ERR_PARAM;
    }

    IpcRet_E enRet = OK;
    cJSON *pNodeData = NULL;
    cJSON *pChildNode = NULL;

    /*创建操作句柄*/
    pNodeData = cJSON_Parse(pJsonBuf);
    if (NULL == pNodeData)
    {
        dlog_error("传入的Json字符串有问题, 无法创建句柄");
        enRet = ERR_PARSE;
        goto EXIT;
    }

    /* 设备id */
    pChildNode = cJSON_GetObjectItem(pNodeData, "DevID");
    if (NULL == pChildNode ||
        NULL == pChildNode->valuestring)
    {
        dlog_error("获取节点[DevID]信息失败");
        enRet = ERR_PARSE;
        goto EXIT;
    }
    stInfo.strDevID = pChildNode->valuestring;

    /* 机器码 */
    pChildNode = cJSON_GetObjectItem(pNodeData, "MachinSn");
    if (NULL == pChildNode ||
        NULL == pChildNode->valuestring)
    {
        dlog_error("获取节点[MachinSn]信息失败");
        enRet = ERR_PARSE;
        goto EXIT;
    }
    stInfo.strMachinSn = pChildNode->valuestring;

    /* 激活/注册码 */
    pChildNode = cJSON_GetObjectItem(pNodeData, "RegisterEg");
    if (NULL == pChildNode ||
        NULL == pChildNode->valuestring)
    {
        dlog_error("获取节点[RegisterEg]信息失败");
        enRet = ERR_PARSE;
        goto EXIT;
    }
    stInfo.strRegisterEg = pChildNode->valuestring,

    /* 激活/注册时间 */
        pChildNode = cJSON_GetObjectItem(pNodeData, "StartTime");
    if (NULL == pChildNode ||
        NULL == pChildNode->valuestring)
    {
        dlog_error("获取节点[StartTime]信息失败");
        enRet = ERR_PARSE;
        goto EXIT;
    }
    stInfo.strStartTime = pChildNode->valuestring;

    /* 最后一次检查的时间 */
    pChildNode = cJSON_GetObjectItem(pNodeData, "PrevTime");
    if (NULL == pChildNode ||
        NULL == pChildNode->valuestring)
    {
        dlog_error("获取节点[PrevTime]信息失败");
        enRet = ERR_PARSE;
        goto EXIT;
    }
    stInfo.strLatestCheckTime = pChildNode->valuestring;

    /* 注册的类型 */
    pChildNode = cJSON_GetObjectItem(pNodeData, "ActionTime");
    if (NULL == pChildNode)
    {
        dlog_error("获取节点[ActionTime]信息失败");
        enRet = ERR_PARSE;
        goto EXIT;
    }
    stInfo.enActionTime = (Register::ActivationTime_E)pChildNode->valueint;

    /* 使用时长，单位：秒（s） */
    pChildNode = cJSON_GetObjectItem(pNodeData, "LifeTimer");
    if (NULL == pChildNode)
    {
        dlog_error("获取节点[LifeTimer]信息失败");
        enRet = ERR_PARSE;
        goto EXIT;
    }
    stInfo.lnLifeTimer = pChildNode->valueint;
    if (stInfo.lnLifeTimer < 0)
    {
        stInfo.lnLifeTimer = 0;
    }

EXIT:
    if (pNodeData)
    {
        cJSON_Delete(pNodeData);
        pNodeData = NULL;
    }

    return enRet;
}

/* 将结构体信息转化为Json数据 */
IpcRet_E CRegisterManage::conver_structToJson(Register::RegisterInfo_S &stInfo, char **pOutJsonBuf)
{
    if (NULL == pOutJsonBuf)
    {
        dlog_error("传入参数异常");
        return ERR_PARAM;
    }

    cJSON *pData = NULL;
    pData = cJSON_CreateObject();

    cJSON_AddStringToObject(pData, "DevID", stInfo.strDevID.c_str());
    cJSON_AddStringToObject(pData, "MachinSn", stInfo.strMachinSn.c_str());
    cJSON_AddStringToObject(pData, "RegisterEg", stInfo.strRegisterEg.c_str());
    cJSON_AddStringToObject(pData, "StartTime", stInfo.strStartTime.c_str());
    cJSON_AddStringToObject(pData, "PrevTime", stInfo.strLatestCheckTime.c_str());
    cJSON_AddNumberToObject(pData, "ActionTime", stInfo.enActionTime);
    if (stInfo.lnLifeTimer < 0)
    {
        cJSON_AddNumberToObject(pData, "LifeTimer", 0);
    }
    else
    {
        cJSON_AddNumberToObject(pData, "LifeTimer", stInfo.lnLifeTimer);
    }

    *pOutJsonBuf = cJSON_Print(pData);

    if (pData)
    {
        cJSON_Delete(pData);
        pData = NULL;
    }

    return OK;
}

/* 读取激活/注册信息配置文件 */
IpcRet_E CRegisterManage::read_registerJson(Register::RegisterInfo_S &stInfo, std::string strFilePath)
{
    IpcRet_E enRetCode = OK;
    struct stat stFileStat;
    memset(&stFileStat, 0, sizeof(stFileStat));
    FILE *pFp = NULL;
    size_t nSize = 0;
    char *pchJsonData = NULL;
    size_t nReadSize = 0;

    /* 获取文件信息 */
    if (stat(strFilePath.c_str(), &stFileStat) != 0)
    {
        dlog_error("文件[%s]信息异常[%s]", strFilePath.c_str(), strerror(errno));
        enRetCode = ERR_FILE_ERR;
        goto EXIT;
    }

    /* 判断路径是否为文件夹 */
    if (S_ISDIR(stFileStat.st_mode))
    {
        dlog_error("传入的文件路径为文件夹[%s]", strFilePath.c_str());
        enRetCode = ERR_FILE_ERR;
        goto EXIT;
    }

    /* 打开文件 */
    pFp = fopen(strFilePath.c_str(), "r");
    if (pFp == NULL)
    {
        dlog_error("打开文件失败[%s]", strFilePath.c_str());
        enRetCode = ERR_OPEN;
        goto EXIT;
    }

    /* 创建空间 */
    nSize = stFileStat.st_size;
    if (nSize == 0)
    {
        /* 空文件属于异常文件，必须返回失败，交由上层走备份恢复逻辑 */
        dlog_info("注册文件json数据未初始化");
        enRetCode = ERR_FREAD;
        goto EXIT;
    }
    /* 额外预留一个字节，保证 Json 解析前字符串以 '\0' 结尾 */
    pchJsonData = new char[nSize + 1];
    if (pchJsonData == NULL)
    {
        dlog_error("创建空间失败");
        enRetCode = ERR_CREATE;
        goto EXIT;
    }

    /* 读取文件 */
    nReadSize = fread(pchJsonData, sizeof(char), nSize, pFp);
    if (nReadSize != nSize)
    {
        dlog_error("读取数据长度异常");
        enRetCode = ERR_FREAD;
        goto EXIT;
    }
    /* fread 不会自动补 '\0'，这里手动补齐，避免解析越界 */
    pchJsonData[nReadSize] = '\0';

    /* 将Json数据转化为结构体信息 */
    enRetCode = conver_jsonToStruct(stInfo, pchJsonData);
    if (enRetCode != OK)
    {
        dlog_error("将Json数据转化为结构体信息-失败");
    }

EXIT:
    if (pchJsonData)
    {
        delete[] pchJsonData;
        pchJsonData = NULL;
    }

    if (pFp)
    {
        fclose(pFp);
        pFp = NULL;
    }

    return enRetCode;
}

/* 写入激活/注册信息配置文件 */
IpcRet_E CRegisterManage::write_registerJson(Register::RegisterInfo_S &stInfo, std::string strFilePath)
{
    IpcRet_E enRetCode = OK;
    char *pchJsonData = NULL;
    FILE *pFp = NULL;
    size_t nLen = 0;
    size_t nWritten = 0;
    /* 临时文件路径，用于原子替换正式文件，避免写到一半留下空文件 */
    std::string strTmpFilePath = strFilePath + ".tmp";
    /* 正式文件所在目录中最后一个'/'的位置 */
    std::string::size_type nPos = std::string::npos;
    /* 正式文件所在目录路径 */
    std::string strDirPath;
    /* 目录文件描述符，用于同步目录项变更 */
    int nDirFd = -1;

    /*确定目录是否存在，不存在则创建目录*/
    create_file_path(strFilePath.c_str());

    struct stat stFileStat;
    memset(&stFileStat, 0, sizeof(stFileStat));

    if (stat(strFilePath.c_str(), &stFileStat) == 0)
    {
        if (S_ISDIR(stFileStat.st_mode))
        {
            dlog_error("写入的路径为文件夹[%s]", strFilePath.c_str());
            enRetCode = ERR_FWRITE;
            goto EXIT;
        }
        else if (access(strFilePath.c_str(), W_OK) != 0)
        {
            if (chmod(strFilePath.c_str(), S_IWUSR) != 0)
            {
                dlog_error("没有写权限，添加写权限失败[%s]", strFilePath.c_str());
                enRetCode = ERR_FWRITE;
                goto EXIT;
            }
        }
    }

    /* 将结构体信息转化为Json数据 */
    enRetCode = conver_structToJson(stInfo, &pchJsonData);
    if (enRetCode != OK)
    {
        dlog_error("将结构体信息转化为Json数据-失败");
        goto EXIT;
    }

    /* 打开文件 */
    pFp = fopen(strTmpFilePath.c_str(), "w");
    if (pFp == NULL)
    {
        dlog_error("打开文件失败[%s]", strTmpFilePath.c_str());
        enRetCode = ERR_FWRITE;
        goto EXIT;
    }

    /* 写入文件 */
    nLen = strlen(pchJsonData);
    nWritten = fwrite(pchJsonData, sizeof(char), nLen, pFp);

    /* 校验写入是否正确 */
    if (nWritten != nLen)
    {
        dlog_error("写文件失败[%s]", strTmpFilePath.c_str());
        enRetCode = ERR_FWRITE;
        goto EXIT;
    }

    if (fflush(pFp) != 0)
    {
        dlog_error("刷新文件失败[%s]", strTmpFilePath.c_str());
        enRetCode = ERR_FWRITE;
        goto EXIT;
    }

    if (fsync(fileno(pFp)) != 0)
    {
        dlog_error("同步文件失败[%s]", strTmpFilePath.c_str());
        enRetCode = ERR_FWRITE;
        goto EXIT;
    }

    if (fclose(pFp) != 0)
    {
        dlog_error("关闭文件失败[%s]", strTmpFilePath.c_str());
        pFp = NULL;
        enRetCode = ERR_FWRITE;
        goto EXIT;
    }
    pFp = NULL;

    /* 临时文件落盘成功后，再原子替换正式文件 */
    if (rename(strTmpFilePath.c_str(), strFilePath.c_str()) != 0)
    {
        dlog_error("替换文件失败[%s]", strFilePath.c_str());
        enRetCode = ERR_FWRITE;
        goto EXIT;
    }

    nPos = strFilePath.rfind('/');
    if (nPos != std::string::npos)
    {
        strDirPath = strFilePath.substr(0, nPos);
        nDirFd = open(strDirPath.c_str(), O_DIRECTORY | O_RDONLY);
        if (nDirFd >= 0)
        {
            fsync(nDirFd);
            close(nDirFd);
        }
    }

EXIT:
    if (pchJsonData)
    {
        free(pchJsonData);
        pchJsonData = NULL;
    }

    if (pFp)
    {
        fclose(pFp);
        pFp = NULL;
    }

    if (enRetCode != OK)
    {
        /* 写入失败时清理临时文件，避免遗留脏文件影响后续排查 */
        unlink(strTmpFilePath.c_str());
    }

    return enRetCode;
}

/* 创建路径中的目录 */
IpcRet_E CRegisterManage::create_file_path(const char *pchFilePath)
{
    char *pchDirPath = strdup(pchFilePath);
    char *pchLastSlash = strrchr(pchDirPath, '/');
    if (pchLastSlash)
    {
        *pchLastSlash = '\0';

        /* 创建目录 */
        mkdir(pchDirPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }
    free(pchDirPath);

    return OK;
}

/* 检查激活/注册码是否被使用 */
IpcRet_E CRegisterManage::check_code_used(std::string strRegisterCode)
{
    /*确定目录是否存在，不存在则创建目录*/
    create_file_path(REGISTER_CODE_MANAGE_PATH);

    int nTotal = 0;
    ini_read_profile_int("Total", "Num", &nTotal, -1, REGISTER_CODE_MANAGE_PATH);

    if (nTotal <= 0)
    {
        return OK_NOT_EXIST;
    }

    dlog_info("已经被使用的激活码数量 %d", nTotal);

    for (int i = 0; i < nTotal; i++)
    {
        char achTmp[64] = {0};
        char achRegisterCode[64] = {0};
        snprintf(achTmp, sizeof(achTmp),
                 "Code%d", i + 1);

        ini_read_profile_char("RegisterCode", achTmp,
                              achRegisterCode, sizeof(achRegisterCode), "", REGISTER_CODE_MANAGE_PATH);

        if (strRegisterCode == std::string(achRegisterCode))
        {
            return OK_EXIST;
        }
    }

    return OK_NOT_EXIST;
}

/* 激活/注册设备并获取激活/注册时间类型 */
IpcRet_E CRegisterManage::set_register(std::string strMachineCode, std::string strRegisterCode, Register::ActivationTime_E &enActionTime)
{
    IpcRet_E enRetCode = OK;

    /* 删除连字符 */
    std::string strResult;
    for (char c : strRegisterCode)
    {
        if (c != '-')
        {
            strResult += c;
        }
    }

    /* 校验激活码 */
    if (strResult.length() != ODE_MAX_LENGTH)
    {
        dlog_error("激活码长度异常 [%d]", strResult.length());
        return ERR_REGISTER_CODE_FAULT;
    }

    /* 激活 */
    for (int i = 0; i < ACTIVATION_TIME_MAX_NUM; i++)
    {
        std::string strTempCode;
        enRetCode = create_register_code(strMachineCode, Register::ActivationTime_E(i), strTempCode);
        if (enRetCode != OK)
        {
            dlog_error("激活失败");
            return enRetCode;
        }

        if (strncmp(strResult.c_str(), strTempCode.c_str(), strResult.length()) == 0)
        {
            dlog_trace("激活成功");
            enActionTime = Register::ActivationTime_E(i);
            return OK;
        }
    }

    /* 激活失败 */
    return ERR_REGISTER_FAULT;
}

/* 生成激活/注册码 */
IpcRet_E CRegisterManage::create_register_code(std::string strMachinSn, Register::ActivationTime_E enTime, std::string &strRegisterCode)
{
    unsigned char achMachineCode[MACHINE_COND_LENGTH + 1] = {0};

    /* 筛选出机器码 */
    int nIndex = 0;
    std::string strResult;
    for (char c : strMachinSn)
    {
        if (c != '-')
        {
            nIndex++;

            /* 去除头EG */
            if (nIndex > MACHINE_HEAD_LENGTH)
            {
                /* 第二层加密算法，简单公式 */
                strResult += TWO_EA(c);
            }
        }
    }

    /* 校验机器码 */
    if (nIndex != MACHINE_HEAD_LENGTH + MACHINE_COND_LENGTH)
    {
        dlog_error("机器码长度异常 [%d][%s]", nIndex, strMachinSn.c_str());
        return ERR_REGISTER_MACHINE_FAULT;
    }

    /* 转化为char数组 */
    strncpy((char *)achMachineCode, strResult.c_str(), strResult.length());

    /* 第二层加密算法，简单公式 */
    achMachineCode[MACHINE_COND_LENGTH] = TWO_EA(enTime);

    /* MD5加密装换 */
    MD5_CTX md5;
    MD5Init(&md5);
    MD5Update(&md5, achMachineCode, sizeof(achMachineCode));
    unsigned char achOut[LENGTH16] = {0};
    MD5Final(&md5, achOut);

    strRegisterCode.clear();
    for (int i = 0; i < 32; i = i + 2)
    {
        /* 对16进制高位转unsigned char */
        strRegisterCode += HexToChar(achOut[i / 2] / 16);
        /* 对16进制低位转unsigned char */
        strRegisterCode += HexToChar(achOut[i / 2] % 16);
    }
    strRegisterCode += '\0';

    return OK;
}

/* 记录激活/注册码 */
IpcRet_E CRegisterManage::write_register_code(std::string strRegisterCode)
{
    int nRet = 0;
    int nTotal = 0;
    ini_read_profile_int("Total", "Num", &nTotal, 0, REGISTER_CODE_MANAGE_PATH);

    char achTmp[64] = {0};
    // char achRegisterCode[64] = {0};
    snprintf(achTmp, sizeof(achTmp), "Code%d", nTotal + 1);

    nRet = ini_write_profile_int("Total", "Num", ++nTotal, REGISTER_CODE_MANAGE_PATH);
    if (nRet == 0)
    {
        dlog_error("更新[%s]中[Total]->[Num]失败", REGISTER_CODE_MANAGE_PATH);
    }

    nRet = ini_write_profile_char("RegisterCode", achTmp, strRegisterCode.c_str(), REGISTER_CODE_MANAGE_PATH);
    if (nRet == 0)
    {
        dlog_error("更新[%s]中[RegisterCode]->[%s]失败", REGISTER_CODE_MANAGE_PATH, achTmp);
    }

    return OK;
}

/* 设置注册信息 */
IpcRet_E CRegisterManage::set_register_info(Register::RegisterInfo_S &stInfo)
{
    IpcRet_E enRetCode = OK;

    /* 清空注册态的局部处理函数，统一复用同一套清理逻辑 */
    auto clear_register_state = [&stInfo]()
    {
        stInfo.strRegisterEg.clear();
        stInfo.strStartTime.clear();
        stInfo.strLatestCheckTime.clear();
        stInfo.lnLifeTimer = 0;
        stInfo.enActionTime = Register::ActivationTime_E::AT_NULL;
    };

    /* 获取机器码和设备ID */
    char achMachinSn[LENGTH64] = {0};
    char achDevID[LENGTH64] = {0};
    enRetCode = get_machine_code(achMachinSn, achDevID);
    if (enRetCode != OK)
    {
        dlog_error("获取机器码和设备ID-失败");
        return enRetCode;
    }

    /* 文件中是否存在完整的设备唯一标识 */
    bool bHasStoredIdentity = !stInfo.strMachinSn.empty() && !stInfo.strDevID.empty();
    /* 文件中的设备唯一标识是否与当前硬件值不一致 */
    bool bIdentityMismatch = strcmp(achMachinSn, stInfo.strMachinSn.c_str()) != 0 ||
                             strcmp(achDevID, stInfo.strDevID.c_str()) != 0;
    /* 文件中是否仍保留注册态相关字段 */
    bool bHasRegisterState = !stInfo.strRegisterEg.empty() || !stInfo.strStartTime.empty() ||
                             !stInfo.strLatestCheckTime.empty() || stInfo.lnLifeTimer > 0 ||
                             stInfo.enActionTime != Register::ActivationTime_E::AT_NULL;

    /* 只有旧文件明确记录了设备唯一标识，且和当前硬件值不一致，才判定为设备被替换 */
    if (bHasStoredIdentity && bIdentityMismatch)
    {
        dlog_error("设备唯一标识被修改, 取消注册码");
        clear_register_state();
    }
    /* 对于缺少唯一标识但仍残留注册态的异常文件，按未注册处理，避免保留脏数据 */
    else if (!bHasStoredIdentity && bHasRegisterState)
    {
        dlog_error("注册信息缺少设备唯一标识, 按未注册处理");
        clear_register_state();
    }

    /* 更新机器码和设备ID */
    stInfo.strMachinSn = achMachinSn;
    stInfo.strDevID = achDevID;

    dlog_trace("当前设备机器码[%s]", stInfo.strMachinSn.c_str());
    dlog_trace("当前的CPUID [%s]", stInfo.strDevID.c_str());
    dlog_trace("当前的激活码 [%s]", stInfo.strRegisterEg.c_str());
    dlog_trace("激活开始时间 [%s]", stInfo.strStartTime.c_str());
    dlog_trace("上一次校验时间 [%s]", stInfo.strLatestCheckTime.c_str());
    dlog_trace("当前激活码类型 [%d]", stInfo.enActionTime);
    dlog_trace("当前可用时间 [%lld]", stInfo.lnLifeTimer);

    /* 写入文件 */
    enRetCode = write_registerJson(stInfo, std::string(REGISTER_INFO_FILE));
    if (enRetCode != OK)
    {
        dlog_error("写入本地文件-失败");
    }

    /* 更新缓存 */
    std::lock_guard<std::mutex> lock(m_registerMutex);
    m_stRegister = stInfo;

    return enRetCode;
}

/* 获取机器码 */
IpcRet_E CRegisterManage::get_machine_code(char *pchMachinSn, char *pchDevID)
{
    if (NULL == pchMachinSn || NULL == pchDevID)
    {
        dlog_error("传入参数异常");
        return ERR_PARAM_NULL;
    }

    std::string strCpuId;
    std::string strMachineCode;
    get_cpu_info(strCpuId, strMachineCode);

    /* 转化为char数组 */
    strncpy(pchDevID, strCpuId.c_str(), strCpuId.length());

    /* 转化为char数组 */
    strncpy(pchMachinSn, strMachineCode.c_str(), strMachineCode.length());

    return OK;
}

/**
 * @description: 获取CPU信息
 * @param [std::string] &strCpuId: CPUID
 * @param [std::string] &strMachineCode: 机器码
 * @return [*] IpcRet_E::OK 成功  其他失败
 * @others:
 */
IpcRet_E CRegisterManage::get_cpu_info(std::string &strCpuId, std::string &strMachineCode)
{
#if 1 /* ==============================第一种方法================================== */
    strCpuId.clear();
    strMachineCode.clear();
    std::ifstream file1(std::string("/proc/cpuinfo"));
    std::string line;
    while (std::getline(file1, line))
    {
        if (line.find("Serial") != std::string::npos)
        {
            std::stringstream ss(line);
            std::string token;
            while (std::getline(ss, token, ':'))
            {
                /* 找到 ":" 后面的字符串 */
                if (!token.empty())
                {
                    ss >> strCpuId;

                    /* 组装机器码 */
                    strMachineCode = std::string("EG-");

                    /* 如果第一组不足5个字符，则在前面添加0 */
                    int nAddNum = 5 - (strCpuId.length() % 5);
                    if (nAddNum != 5)
                    {
                        for (int i = 0; i < nAddNum; i++)
                        {
                            strCpuId = "0" + strCpuId;
                        }
                    }

                    for (size_t i = 0; i < strCpuId.length(); i++)
                    {
                        strMachineCode += strCpuId[i];
                        if ((i + 1) % 5 == 0 && i != strCpuId.length() - 1)
                        {
                            strMachineCode += "-";
                        }
                    }

                    file1.close();
                    return OK;
                }
            }
        }
    }
    file1.close();
#endif /* ==============================第一种方法================================== */

#if 1 /* ==============================第二种方法================================== */
    strCpuId.clear();
    strMachineCode.clear();
    /* 检查是否存在 uuid.txt 文件 */
    std::ifstream file2(UUID_FILE_PATH);
    if (file2.good())
    {
        std::stringstream buffer;
        buffer << file2.rdbuf();
        strCpuId = buffer.str();
        file2.close();

        /* 移除末尾的换行符 */
        if (!strCpuId.empty() && strCpuId.back() == '\n')
        {
            strCpuId.pop_back();
        }
    }

    if (strCpuId.empty())
    {
        /* 生成新的 UUID */
        FILE *pPipe = popen("uuidgen", "r");
        if (pPipe)
        {
            char achBuffer[1024] = {0};
            while (!feof(pPipe))
            {
                if (fgets(achBuffer, 1024, pPipe) != NULL)
                {
                    strCpuId += achBuffer;
                }
            }
            pclose(pPipe);

            /* 移除末尾的换行符 */
            if (!strCpuId.empty() && strCpuId.back() == '\n')
            {
                strCpuId.pop_back();
            }

            /* 将生成的 UUID 写入 uuid.txt 文件 */
            std::ofstream outputFile(UUID_FILE_PATH, std::ios::trunc);
            if (outputFile.is_open())
            {
                outputFile << strCpuId;
                outputFile.close();
            }
        }
    }

    if (!strCpuId.empty())
    {
        /* 组装机器码 */
        strMachineCode = std::string("EG-");
        std::stringstream ss(strCpuId);
        std::string strSegment;
        int nSegmentCount = 0;

        while (std::getline(ss, strSegment, '-'))
        {
            /* 取前 5 个字符并在不足 5 个字符时补零 */
            std::string segmentPrefix = strSegment.substr(0, 5);
            if (segmentPrefix.size() < 5)
            {
                segmentPrefix += std::string(5 - segmentPrefix.size(), '0');
            }

            /* 添加到机器码中 */
            strMachineCode += segmentPrefix;

            /* 添加分隔符 */
            strMachineCode += "-";
            nSegmentCount++;
            if (nSegmentCount >= 4)
            {
                break;
            }
        }

        /* 移除最后一个 "-" */
        if (!strMachineCode.empty())
        {
            strMachineCode.pop_back();
        }
        return OK;
    }
#endif /* ==============================第二种方法================================== */

    /* 其他方法 */
    strCpuId.clear();
    strMachineCode.clear();

    return ERR;
}
