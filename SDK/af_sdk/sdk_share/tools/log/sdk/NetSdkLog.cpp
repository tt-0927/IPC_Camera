/**
 * @file NetSdkLog.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief NetSdkLog 模块实现
 * 功能说明：
 * 1. 实现 NetSdkLog 模块核心逻辑
 * 2. 校验输入参数并管理模块资源生命周期
 * 3. 向上层提供可复用的 SDK 能力
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <map>
#include <memory>
#include <mutex>
#include <iostream>
#include <string>
#include <time.h>
#include "NetSdkLog.h"
#include "spdlog/spdlog.h"
#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/sinks/null_sink.h"
#include "spdlog/sinks/ostream_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/pattern_formatter.h"
#include "spdlog/sinks/dist_sink.h"

#define NETSDK_LOG_FILENAME     "NET_LOG.log"

namespace
{
struct DlogInnerHandle_S
{
    /* 使用 map 管理多个 logger，key 为 logger 名称。 */
    std::map<std::string, std::shared_ptr<spdlog::logger>> m_stLoggers;

    /* 默认 logger，用于未匹配到专用 logger 的情况。 */
    std::shared_ptr<spdlog::logger> m_pDefaultLogger;

    bool m_bSyncPrintf{false};
};

std::recursive_mutex gs_stLogMutex;
std::shared_ptr<DlogInnerHandle_S> gs_pLogInnerHandle;
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 SdkLogGetHandle 定义的内部处理。
 * @return 返回该处理的状态或结果。
 */

static std::shared_ptr<DlogInnerHandle_S> SdkLogGetHandle()
{
    std::lock_guard<std::recursive_mutex> stLock(gs_stLogMutex);
    return gs_pLogInnerHandle;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 SdkLogEnsureHandle 定义的内部处理。
 * @return 返回该处理的状态或结果。
 */

static std::shared_ptr<DlogInnerHandle_S> SdkLogEnsureHandle()
{
    std::lock_guard<std::recursive_mutex> stLock(gs_stLogMutex);
    if (!gs_pLogInnerHandle)
    {
        gs_pLogInnerHandle = std::make_shared<DlogInnerHandle_S>();
    }
    return gs_pLogInnerHandle;
}
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 initSdkLog 对应的处理。
 * @param [in,out] pstrLogName 函数处理参数。
 * @param [in,out] pstrLogFile 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

int initSdkLog(char* pstrLogName,char* pstrLogFile)
{
    std::lock_guard<std::recursive_mutex> stLock(gs_stLogMutex);
    const std::shared_ptr<DlogInnerHandle_S> pLogHandle = SdkLogEnsureHandle();

	int nMaxDays = 7;
	std::string logName = pstrLogName;/*"logger"; */
	std::string logFile = pstrLogFile;/*"vss.log"; */


	auto newLogger = spdlog::daily_logger_mt(logName, logFile, 0, 0, false, nMaxDays);

	/* [][%@,%!] */
	spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%s:%#,%!] [%l] %v");

	/* 设置日志等级 */
	spdlog::set_level(spdlog::level::trace);

	/* 当遇到warn消息级别以上的立刻刷新到日志 */
    newLogger->flush_on(spdlog::level::info);

    /* 保存 logger */
    pLogHandle->m_stLoggers[logName] = newLogger;

    /* 如果是第一个 logger，设为默认 */
    if (!pLogHandle->m_pDefaultLogger) {
        pLogHandle->m_pDefaultLogger = newLogger;
    }

	return 0;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 initSdkLogBySize 对应的处理。
 * @param [in,out] pstrLogName 函数处理参数。
 * @param [in,out] pstrLogFile 函数处理参数。
 * @param [in] nMaxFileSize 函数处理参数。
 * @param [in] nMaxFiles 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

int initSdkLogBySize(char *pstrLogName, char *pstrLogFile, int nMaxFileSize, int nMaxFiles)
{
    std::lock_guard<std::recursive_mutex> stLock(gs_stLogMutex);
    const std::shared_ptr<DlogInnerHandle_S> pLogHandle = SdkLogEnsureHandle();

	std::string logName = pstrLogName;/*"logger"; */
	std::string logFile = pstrLogFile;/*"vss.log"; */

	/* 在日志文件名中添加日期 */
	time_t lNow = time(NULL);
	struct tm *pstTime = localtime(&lNow);
	char strDate[32];
	strftime(strDate, sizeof(strDate), "_%Y-%m-%d", pstTime);

	/* 找到文件扩展名位置，在扩展名前插入日期 */
	size_t uDotPos = logFile.find_last_of('.');
	if (uDotPos != std::string::npos) {
		logFile.insert(uDotPos, strDate);
	} else {
		logFile += strDate;
	}

	auto newLogger = spdlog::rotating_logger_mt(logName, logFile, nMaxFileSize, nMaxFiles);

    /* [][%@,%!] */
	spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%s:%#,%!] [%l] %v");

	/* 设置日志等级 */
	spdlog::set_level(spdlog::level::trace);

	/* 当遇到warn消息级别以上的立刻刷新到日志 */
    newLogger->flush_on(spdlog::level::info);

    /* 保存 logger */
    pLogHandle->m_stLoggers[logName] = newLogger;

    /* 如果是第一个 logger，设为默认 */
    if (!pLogHandle->m_pDefaultLogger) {
        pLogHandle->m_pDefaultLogger = newLogger;
    }

	return 0;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 uninitSdkLog 定义的内部处理。
 * @return 返回该处理的状态或结果。
 */

int uninitSdkLog()
{
    std::lock_guard<std::recursive_mutex> stLock(gs_stLogMutex);
    /* 首先判断基础日志创建了没有 */
	if(!gs_pLogInnerHandle)
	{
		printf("no init log base!!!!\n");
		return -1;
	}

    /* 关闭并注销所有 logger。 */
    spdlog::drop_all();
    gs_pLogInnerHandle.reset();

	return 0;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 setLogLevel 对应的处理。
 * @param [in] nLevel 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

int setLogLevel(int nLevel)
{
    std::lock_guard<std::recursive_mutex> stLock(gs_stLogMutex);
    /* 首先判断基础日志创建了没有 */
	if(!gs_pLogInnerHandle)
	{
		printf("no init log base!!!!\n");
		return -1;
	}

	/* 设置日志等级  */
    spdlog::level::level_enum enLevel = spdlog::level::trace;
	switch(nLevel)
	{
		case NETSDK_LOG_TRACE:
            enLevel = spdlog::level::trace;
			break;
		case NETSDK_LOG_DEBUG:
            enLevel = spdlog::level::debug;
			break;
		case NETSDK_LOG_INFO:
            enLevel = spdlog::level::info;
			break;
		case NETSDK_LOG_WARN:
            enLevel = spdlog::level::warn;
			break;
		case NETSDK_LOG_ERROR:
            enLevel = spdlog::level::err;
			break;
	}

    spdlog::set_level(enLevel);

    for (auto& pair : gs_pLogInnerHandle->m_stLoggers) {
        if (pair.second) {
            pair.second->set_level(enLevel);
        }
    }
	return 0;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 createLevelLog 对应的处理。
 * @param [in,out] pLogHandle 函数处理参数。
 * @param [in,out] pstrLogName 函数处理参数。
 * @param [in,out] pstrLogFile 函数处理参数。
 * @param [in] nLevel 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

static int createLevelLog(DlogInnerHandle_S* pLogHandle, char* pstrLogName, char* pstrLogFile, int nLevel)
{
	if(pLogHandle == nullptr)
	{
		printf("this argument is NULL!!!\n");
		return -1;
	}

	int nMaxDays = 7;
	std::string logName = pstrLogName;/*"logger"; */
	std::string logFile = pstrLogFile;/*"vss.log"; */

	switch(nLevel)
	{
		case NETSDK_LOG_WARN:
		{
            auto logger = spdlog::daily_logger_mt(logName, logFile, 0, 0, false, nMaxDays);
            logger->set_pattern("[%H:%M:%S.%e] [%s:%#,%!] [%l] %v");
			pLogHandle->m_stLoggers[logName] = logger;
            /* Also register with a suffix if needed, or rely on distinct logName */
			break;
		}
		case NETSDK_LOG_ERROR:
		{
            auto logger = spdlog::daily_logger_mt(logName, logFile, 0, 0, false, nMaxDays);
            logger->set_pattern("[%H:%M:%S.%e] [%s:%#,%!] [%l] %v");
			pLogHandle->m_stLoggers[logName] = logger;
			break;
		}
		default:
			printf("this level[%d] is not support!!!\n",nLevel);
			return -1;
	}

    /* 单独设置每个日志级别的输出格式。 */
/*    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] %@ %! [%l] %v"); */

	return 0;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 createLevelLogBySize 对应的处理。
 * @param [in,out] pLogHandle 函数处理参数。
 * @param [in,out] pstrLogName 函数处理参数。
 * @param [in,out] pstrLogFile 函数处理参数。
 * @param [in] nLevel 函数处理参数。
 * @param [in] nMaxFileSize 函数处理参数。
 * @param [in] nMaxFiles 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

static int createLevelLogBySize(DlogInnerHandle_S* pLogHandle, char* pstrLogName, char* pstrLogFile, int nLevel, int nMaxFileSize, int nMaxFiles)
{
	if(pLogHandle == nullptr)
	{
		printf("this argument is NULL!!!\n");
		return -1;
	}

	std::string logName = pstrLogName;
	std::string logFile = pstrLogFile;

	switch(nLevel)
	{
		case NETSDK_LOG_WARN:
		{
            auto logger = spdlog::rotating_logger_mt(logName, logFile, nMaxFileSize, nMaxFiles);
            logger->set_pattern("[%H:%M:%S.%e] [%s:%#,%!] [%l] %v");
			pLogHandle->m_stLoggers[logName] = logger;
			break;
		}
		case NETSDK_LOG_ERROR:
		{
            auto logger = spdlog::rotating_logger_mt(logName, logFile, nMaxFileSize, nMaxFiles);
            logger->set_pattern("[%H:%M:%S.%e] [%s:%#,%!] [%l] %v");
			pLogHandle->m_stLoggers[logName] = logger;
			break;
		}
		default:
			printf("this level[%d] is not support!!!\n",nLevel);
			return -1;
	}
	return 0;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 initSdkLogLevel 对应的处理。
 * @param [in,out] pstrLogName 函数处理参数。
 * @param [in,out] pstrLogFile 函数处理参数。
 * @param [in] nLevel 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

int initSdkLogLevel(char *pstrLogName, char *pstrLogFile, int nLevel)
{
    std::lock_guard<std::recursive_mutex> stLock(gs_stLogMutex);
    /* 首先判断基础日志创建了没有 */
	if(!gs_pLogInnerHandle)
	{
		printf("no init log base!!!!\n");
		return -1;
	}

	/* 再创建对应等级的日志文件 */
	int nRet = 0;
	nRet = createLevelLog(gs_pLogInnerHandle.get(),\
							pstrLogName,pstrLogFile,nLevel);
	return nRet;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 initSdkLogLevelBySize 对应的处理。
 * @param [in,out] pstrLogName 函数处理参数。
 * @param [in,out] pstrLogFile 函数处理参数。
 * @param [in] nLevel 函数处理参数。
 * @param [in] nMaxFileSize 函数处理参数。
 * @param [in] nMaxFiles 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

int initSdkLogLevelBySize(char *pstrLogName, char *pstrLogFile, int nLevel, int nMaxFileSize, int nMaxFiles)
{
    std::lock_guard<std::recursive_mutex> stLock(gs_stLogMutex);
    /* 首先判断基础日志创建了没有 */
    if (!gs_pLogInnerHandle)
    {
		printf("no init log base!!!!\n");
		return -1;
	}

	/* 判断参数是否正确 */
    if (pstrLogName == NULL || pstrLogFile == NULL || nMaxFileSize == 0 || nMaxFiles == 0)
    {
		printf("The parameters are incorrect!!!!\n");
		return -1;
	}

	/* 再创建对应等级的日志文件 */
	int nRet = 0;
	nRet = createLevelLogBySize(gs_pLogInnerHandle.get(), pstrLogName, pstrLogFile, nLevel, nMaxFileSize, nMaxFiles);
    return nRet;
}

/* 查找并跳过ANSI转义序列 */
void skip_ansi_escape_sequences(char *pstrStr)
{
    char *pstrStart = pstrStr;
    while (*pstrStart)
	{
        if (pstrStart[0] == '\x1B' && pstrStart[1] == '[')
		{
            /* 跳过ANSI转义序列 */
            pstrStart += 2;
            while (*pstrStart && *pstrStart != 'm')
			{
                pstrStart++;
            }
            /* 跳过m字符 */
            pstrStart++;
        }
        else
        {
            /* 如果是普通字符，复制到新字符串 */
            char *pstrDest = pstrStr;
            while (*pstrStart && *pstrStart != '\x1B')
			{
                *pstrDest++ = *pstrStart++;
            }
			/* 添加字符串终止符 */
            *pstrDest = '\0';
            return;
        }
    }
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 netSdk_log 定义的内部处理。
 * @return 返回该处理的状态或结果。
 */

int netSdk_log(int nLevel,\
		const char *pstrFileName, int nLine, const char *pstrFuncName,\
		const char *pstrFormat, ...)
{
	const std::shared_ptr<DlogInnerHandle_S> pLogHandle = SdkLogGetHandle();

    std::string strFileName  = pstrFileName;

	if(!pLogHandle)
	{
		/*return 0; */
		/* 没有初始化日志，默认输出到控制台 */
		va_list ap;
		va_start(ap, pstrFormat);
		printf("[%s:%d,%s] ", strFileName.c_str(), nLine,pstrFuncName);
		vprintf(pstrFormat, ap);
		printf("\n");
		va_end(ap);
	}else
	{
		char* pstrFmt = NULL;
		char defaultStr[4096] = {0};
		char strMsg[4096] = {0};
		char sendMsg[8192] = {0};
		/* int nLen = 0; */
		std::string str;

		va_list args0, args1;
		va_start(args0, pstrFormat);
		va_copy(args1, args0);
		size_t uNumOfChars = std::vsnprintf(nullptr, 0, pstrFormat, args0);

		va_end(args0);

		if((uNumOfChars+1) > 4096)
		{
			str.resize(uNumOfChars + 1, '\0');
			std::vsnprintf(const_cast<char*>(str.data()), str.size(), pstrFormat, args1);
			str.resize(uNumOfChars);
			pstrFmt = const_cast<char*>(str.data());
		}else
		{
			std::vsnprintf(defaultStr, uNumOfChars+1, pstrFormat, args1);
			pstrFmt = defaultStr;

			strncpy(strMsg, defaultStr, sizeof(strMsg));
			skip_ansi_escape_sequences(strMsg);
			snprintf(sendMsg, sizeof(sendMsg), "%s", strMsg);
		}

		va_end(args1);

        std::shared_ptr<spdlog::logger> pTargetLogger;
        bool bSyncPrintf = false;
        {
            std::lock_guard<std::recursive_mutex> stLock(gs_stLogMutex);
            pTargetLogger = pLogHandle->m_pDefaultLogger;
            bSyncPrintf = pLogHandle->m_bSyncPrintf;

            /* 根据源文件所属模块选择专用 logger。 */
            if (strFileName.find("sdk_server") != std::string::npos || strFileName.find("SDK_SERVER") != std::string::npos) {
                const auto stIterator = pLogHandle->m_stLoggers.find("NetTVSDKServer");
                if (stIterator != pLogHandle->m_stLoggers.end()) {
                    pTargetLogger = stIterator->second;
                }
            }
            else if (strFileName.find("sdk_client") != std::string::npos || strFileName.find("SDK_CLIENT") != std::string::npos) {
                const auto stIterator = pLogHandle->m_stLoggers.find("NetTVSDKClient");
                if (stIterator != pLogHandle->m_stLoggers.end()) {
                    pTargetLogger = stIterator->second;
                }
            }
        }

        if (pTargetLogger) {
            switch(nLevel)
            {
            case NETSDK_LOG_TRACE:
                pTargetLogger->log(spdlog::source_loc{strFileName.c_str(), nLine, pstrFuncName}, spdlog::level::trace, pstrFmt);
                break;
            case NETSDK_LOG_DEBUG:
                pTargetLogger->log(spdlog::source_loc{strFileName.c_str(), nLine, pstrFuncName}, spdlog::level::debug, pstrFmt);
                break;
            case NETSDK_LOG_INFO:
                pTargetLogger->log(spdlog::source_loc{strFileName.c_str(), nLine, pstrFuncName}, spdlog::level::info, pstrFmt);
                break;
            case NETSDK_LOG_WARN:
                pTargetLogger->log(spdlog::source_loc{strFileName.c_str(), nLine, pstrFuncName}, spdlog::level::warn, pstrFmt);
                break;
            case NETSDK_LOG_ERROR:
                pTargetLogger->log(spdlog::source_loc{strFileName.c_str(), nLine, pstrFuncName}, spdlog::level::err, pstrFmt);
                break;
            }
			/* 手动刷新确保日志写入文件。 */
            pTargetLogger->flush();
        }


#ifndef RELEASE_VERSION
		/*Debug版默认同步输出打印*/
		if (pLogHandle)
		{
			char timeBuf[64];
			getCurrTime(timeBuf, NULL);
			size_t uLastSlash = strFileName.find_last_of("/\\");
			std::string strFileNameOnly = (uLastSlash != std::string::npos) ? strFileName.substr(uLastSlash + 1) : strFileName;
			printf("%s[%s:%d] %s\n", timeBuf, strFileNameOnly.c_str(), nLine, pstrFmt);
			fflush(stdout);
		}
#else
		/*Release版需手动启用同步输出打印*/
		if (pLogHandle && bSyncPrintf)
		{
			char timeBuf[64];
			getCurrTime(timeBuf, NULL);
			printf("%s[%s:%d] %s\n", timeBuf, strFileName.c_str(), nLine, pstrFmt);
			fflush(stdout);
		}
#endif

	}

	return 0;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 syncPrintf 定义的内部处理。
 * @param [in] bStatus 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

int syncPrintf(bool bStatus)
{
    std::lock_guard<std::recursive_mutex> stLock(gs_stLogMutex);
    if(!gs_pLogInnerHandle){
        printf("no init log base!!!!\n");
        return -1;
    }
    gs_pLogInnerHandle->m_bSyncPrintf = bStatus;
    return 0;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 查询或校验 getCurrTime 对应的数据。
 * @param [out] pstrOutputBuf 函数处理参数。
 * @param [in,out] pstrTimeFormat 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

int getCurrTime(char* pstrOutputBuf,char* pstrTimeFormat)
{
	if(pstrOutputBuf == NULL){
		return -1;/*无输出源进入*/
	}
	/*默认格式*/
	char strFormat[32] = "[%Y-%m-%d %H:%M:%S.%e]";
	if(pstrTimeFormat == NULL){
		pstrTimeFormat = strFormat;
	}
	struct tm *pstNewTime;
	time_t lTime;
	time(&lTime);
	pstNewTime = localtime(&lTime);
	strftime(pstrOutputBuf, 64, pstrTimeFormat, pstNewTime);
    return 0;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 setFlushLevel 对应的处理。
 * @param [in] nLevel 函数处理参数。
 * @return 返回该处理的状态或结果。
 */
int setFlushLevel(int nLevel)
{
    std::lock_guard<std::recursive_mutex> stLock(gs_stLogMutex);
    /* 首先判断基础日志创建了没有 */
    if(!gs_pLogInnerHandle)
    {
        printf("no init log base!!!!\n");
        return -1;
    }

    /*默认警告等级及以上才立即刷新*/
    /*支持调低刷新等级，不支持调高刷新等级*/
    spdlog::level::level_enum enLevel = spdlog::level::warn;
    switch (nLevel) {
    case NETSDK_LOG_TRACE:enLevel = spdlog::level::trace;break;
    case NETSDK_LOG_DEBUG:enLevel = spdlog::level::debug;break;
    case NETSDK_LOG_INFO:enLevel = spdlog::level::info;break;
    default:break;
    }


    for (auto& pair : gs_pLogInnerHandle->m_stLoggers) {
        if (pair.second) {
            pair.second->flush_on(enLevel);
        }
    }

    return 0;
}
