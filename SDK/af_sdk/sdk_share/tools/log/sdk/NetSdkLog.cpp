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

#define NETSDK_LOG_FILENAME     "NET_TV_LOG.log"

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
 * @param [in,out] logname 函数处理参数。
 * @param [in,out] logfile 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

int initSdkLog(char* logname,char* logfile)
{
    std::lock_guard<std::recursive_mutex> stLock(gs_stLogMutex);
    const std::shared_ptr<DlogInnerHandle_S> pLogHandle = SdkLogEnsureHandle();

	int max_days = 7;
	std::string logName = logname;/*"logger"; */
	std::string logFile = logfile;/*"vss.log"; */


	auto newLogger = spdlog::daily_logger_mt(logName, logFile, 0, 0, false, max_days);

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
 * @param [in,out] logname 函数处理参数。
 * @param [in,out] logfile 函数处理参数。
 * @param [in] max_file_size 函数处理参数。
 * @param [in] max_files 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

int initSdkLogBySize(char *logname, char *logfile, int max_file_size, int max_files)
{
    std::lock_guard<std::recursive_mutex> stLock(gs_stLogMutex);
    const std::shared_ptr<DlogInnerHandle_S> pLogHandle = SdkLogEnsureHandle();

	std::string logName = logname;/*"logger"; */
	std::string logFile = logfile;/*"vss.log"; */

	/* 在日志文件名中添加日期 */
	time_t now = time(NULL);
	struct tm *t = localtime(&now);
	char dateStr[32];
	strftime(dateStr, sizeof(dateStr), "_%Y-%m-%d", t);

	/* 找到文件扩展名位置，在扩展名前插入日期 */
	size_t dotPos = logFile.find_last_of('.');
	if (dotPos != std::string::npos) {
		logFile.insert(dotPos, dateStr);
	} else {
		logFile += dateStr;
	}

	auto newLogger = spdlog::rotating_logger_mt(logName, logFile, max_file_size, max_files);

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
 * @param [in] level 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

int setLogLevel(int level)
{
    std::lock_guard<std::recursive_mutex> stLock(gs_stLogMutex);
    /* 首先判断基础日志创建了没有 */
	if(!gs_pLogInnerHandle)
	{
		printf("no init log base!!!!\n");
		return -1;
	}

	/* 设置日志等级  */
    spdlog::level::level_enum spdLevel = spdlog::level::trace;
	switch(level)
	{
		case NETSDK_LOG_TRACE:
            spdLevel = spdlog::level::trace;
			break;
		case NETSDK_LOG_DEBUG:
            spdLevel = spdlog::level::debug;
			break;
		case NETSDK_LOG_INFO:
            spdLevel = spdlog::level::info;
			break;
		case NETSDK_LOG_WARN:
            spdLevel = spdlog::level::warn;
			break;
		case NETSDK_LOG_ERROR:
            spdLevel = spdlog::level::err;
			break;
	}

    spdlog::set_level(spdLevel);

    for (auto& pair : gs_pLogInnerHandle->m_stLoggers) {
        if (pair.second) {
            pair.second->set_level(spdLevel);
        }
    }
	return 0;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 createLevelLog 对应的处理。
 * @param [in,out] pLogHandle 函数处理参数。
 * @param [in,out] logname 函数处理参数。
 * @param [in,out] logfile 函数处理参数。
 * @param [in] level 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

static int createLevelLog(DlogInnerHandle_S* pLogHandle, char* logname, char* logfile, int level)
{
	if(pLogHandle == nullptr)
	{
		printf("this argument is NULL!!!\n");
		return -1;
	}

	int max_days = 7;
	std::string logName = logname;/*"logger"; */
	std::string logFile = logfile;/*"vss.log"; */

	switch(level)
	{
		case NETSDK_LOG_WARN:
		{
            auto logger = spdlog::daily_logger_mt(logName, logFile, 0, 0, false, max_days);
            logger->set_pattern("[%H:%M:%S.%e] [%s:%#,%!] [%l] %v");
			pLogHandle->m_stLoggers[logName] = logger;
            /* Also register with a suffix if needed, or rely on distinct logName */
			break;
		}
		case NETSDK_LOG_ERROR:
		{
            auto logger = spdlog::daily_logger_mt(logName, logFile, 0, 0, false, max_days);
            logger->set_pattern("[%H:%M:%S.%e] [%s:%#,%!] [%l] %v");
			pLogHandle->m_stLoggers[logName] = logger;
			break;
		}
		default:
			printf("this level[%d] is not support!!!\n",level);
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
 * @param [in,out] logname 函数处理参数。
 * @param [in,out] logfile 函数处理参数。
 * @param [in] level 函数处理参数。
 * @param [in] max_file_size 函数处理参数。
 * @param [in] max_files 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

static int createLevelLogBySize(DlogInnerHandle_S* pLogHandle, char* logname, char* logfile, int level, int max_file_size, int max_files)
{
	if(pLogHandle == nullptr)
	{
		printf("this argument is NULL!!!\n");
		return -1;
	}

	std::string logName = logname;
	std::string logFile = logfile;

	switch(level)
	{
		case NETSDK_LOG_WARN:
		{
            auto logger = spdlog::rotating_logger_mt(logName, logFile, max_file_size, max_files);
            logger->set_pattern("[%H:%M:%S.%e] [%s:%#,%!] [%l] %v");
			pLogHandle->m_stLoggers[logName] = logger;
			break;
		}
		case NETSDK_LOG_ERROR:
		{
            auto logger = spdlog::rotating_logger_mt(logName, logFile, max_file_size, max_files);
            logger->set_pattern("[%H:%M:%S.%e] [%s:%#,%!] [%l] %v");
			pLogHandle->m_stLoggers[logName] = logger;
			break;
		}
		default:
			printf("this level[%d] is not support!!!\n",level);
			return -1;
	}
	return 0;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 initSdkLogLevel 对应的处理。
 * @param [in,out] logname 函数处理参数。
 * @param [in,out] logfile 函数处理参数。
 * @param [in] level 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

int initSdkLogLevel(char *logname, char *logfile, int level)
{
    std::lock_guard<std::recursive_mutex> stLock(gs_stLogMutex);
    /* 首先判断基础日志创建了没有 */
	if(!gs_pLogInnerHandle)
	{
		printf("no init log base!!!!\n");
		return -1;
	}

	/* 再创建对应等级的日志文件 */
	int ret = 0;
	ret = createLevelLog(gs_pLogInnerHandle.get(),\
							logname,logfile,level);
	return ret;
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 initSdkLogLevelBySize 对应的处理。
 * @param [in,out] logname 函数处理参数。
 * @param [in,out] logfile 函数处理参数。
 * @param [in] level 函数处理参数。
 * @param [in] max_file_size 函数处理参数。
 * @param [in] max_files 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

int initSdkLogLevelBySize(char *logname, char *logfile, int level, int max_file_size, int max_files)
{
    std::lock_guard<std::recursive_mutex> stLock(gs_stLogMutex);
    /* 首先判断基础日志创建了没有 */
    if (!gs_pLogInnerHandle)
    {
		printf("no init log base!!!!\n");
		return -1;
	}

	/* 判断参数是否正确 */
    if (logname == NULL || logfile == NULL || max_file_size == 0 || max_files == 0)
    {
		printf("The parameters are incorrect!!!!\n");
		return -1;
	}

	/* 再创建对应等级的日志文件 */
	int ret = 0;
	ret = createLevelLogBySize(gs_pLogInnerHandle.get(), logname, logfile, level, max_file_size, max_files);
    return ret;
}

/* 查找并跳过ANSI转义序列 */
void skip_ansi_escape_sequences(char *str)
{
    char *start = str;
    while (*start)
	{
        if (start[0] == '\x1B' && start[1] == '[')
		{
            /* 跳过ANSI转义序列 */
            start += 2;
            while (*start && *start != 'm')
			{
                start++;
            }
            /* 跳过m字符 */
            start++;
        }
        else
        {
            /* 如果是普通字符，复制到新字符串 */
            char *dest = str;
            while (*start && *start != '\x1B')
			{
                *dest++ = *start++;
            }
			/* 添加字符串终止符 */
            *dest = '\0';
            return;
        }
    }
}
/**
 * @author tianl (tianl@kfb.cn)
 * @brief 执行 netSdk_log 定义的内部处理。
 * @return 返回该处理的状态或结果。
 */

int netSdk_log(int level,\
		const char *filename_in, int line_in, const char *funcname_in,\
		const char *format, ...)
{
	const std::shared_ptr<DlogInnerHandle_S> pLogHandle = SdkLogGetHandle();

    std::string strFileName  = filename_in;

	if(!pLogHandle)
	{
		/*return 0; */
		/* 没有初始化日志，默认输出到控制台 */
		va_list ap;
		va_start(ap, format);
		printf("[%s:%d,%s] ", strFileName.c_str(), line_in,funcname_in);
		vprintf(format, ap);
		printf("\n");
		va_end(ap);
	}else
	{
		char* ptrFmt = NULL;
		char defaultStr[4096] = {0};
		char strMsg[4096] = {0};
		char sendMsg[8192] = {0};
		/* int nLen = 0; */
		std::string str;

		va_list args0, args1;
		va_start(args0, format);
		va_copy(args1, args0);
		size_t num_of_chars = std::vsnprintf(nullptr, 0, format, args0);

		va_end(args0);

		if((num_of_chars+1) > 4096)
		{
			str.resize(num_of_chars + 1, '\0');
			std::vsnprintf(const_cast<char*>(str.data()), str.size(), format, args1);
			str.resize(num_of_chars);
			ptrFmt = const_cast<char*>(str.data());
		}else
		{
			std::vsnprintf(defaultStr, num_of_chars+1, format, args1);
			ptrFmt = defaultStr;

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
            switch(level)
            {
            case NETSDK_LOG_TRACE:
                pTargetLogger->log(spdlog::source_loc{strFileName.c_str(), line_in, funcname_in}, spdlog::level::trace, ptrFmt);
                break;
            case NETSDK_LOG_DEBUG:
                pTargetLogger->log(spdlog::source_loc{strFileName.c_str(), line_in, funcname_in}, spdlog::level::debug, ptrFmt);
                break;
            case NETSDK_LOG_INFO:
                pTargetLogger->log(spdlog::source_loc{strFileName.c_str(), line_in, funcname_in}, spdlog::level::info, ptrFmt);
                break;
            case NETSDK_LOG_WARN:
                pTargetLogger->log(spdlog::source_loc{strFileName.c_str(), line_in, funcname_in}, spdlog::level::warn, ptrFmt);
                break;
            case NETSDK_LOG_ERROR:
                pTargetLogger->log(spdlog::source_loc{strFileName.c_str(), line_in, funcname_in}, spdlog::level::err, ptrFmt);
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
			size_t lastSlash = strFileName.find_last_of("/\\");
			std::string fileNameOnly = (lastSlash != std::string::npos) ? strFileName.substr(lastSlash + 1) : strFileName;
			printf("%s[%s:%d] %s\n", timeBuf, fileNameOnly.c_str(), line_in, ptrFmt);
			fflush(stdout);
		}
#else
		/*Release版需手动启用同步输出打印*/
		if (pLogHandle && bSyncPrintf)
		{
			char timeBuf[64];
			getCurrTime(timeBuf, NULL);
			printf("%s[%s:%d] %s\n", timeBuf, strFileName.c_str(), line_in, ptrFmt);
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
 * @param [out] outputBuf 函数处理参数。
 * @param [in,out] timeFormat 函数处理参数。
 * @return 返回该处理的状态或结果。
 */

int getCurrTime(char* outputBuf,char* timeFormat)
{
	if(outputBuf == NULL){
		return -1;/*无输出源进入*/
	}
	/*默认格式*/
	char format[32] = "[%Y-%m-%d %H:%M:%S.%e]";
	if(timeFormat == NULL){
		timeFormat = format;
	}
	struct tm *newtime;
	time_t lt1;
	time(&lt1);
	newtime = localtime(&lt1);
	strftime(outputBuf, 64, timeFormat, newtime);
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
