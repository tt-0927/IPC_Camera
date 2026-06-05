/**
 * @FilePath     : dlog.cpp
 * @Author       : zhangjunbin
 * @Date         : 2021年3月30日
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-10-16 09:19:08
 * @Description  : 日志的基础库，基于spdlog封装
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <map>
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

typedef struct dlogInnerHandle
{
    /* 使用 map 管理多个 logger，key 为 logger 名称 */
    std::map<std::string, std::shared_ptr<spdlog::logger>> loggers;
    
    // 默认 logger，用于未匹配到的情况
    std::shared_ptr<spdlog::logger> defaultLogger;

    bool bSynPrintf;
}dlogInnerHandle_S;

dlogHandle_S g_dlogInnerHandle = NULL;

int initSdkLog(char* logname,char* logfile)
{
    if (g_dlogInnerHandle == NULL)
    {
        g_dlogInnerHandle = new dlogInnerHandle_S;
        ((dlogInnerHandle_S*)g_dlogInnerHandle)->bSynPrintf = false;
    }
    dlogInnerHandle_S* handle = (dlogInnerHandle_S*)g_dlogInnerHandle;

	int max_days = 7;
	std::string logName = logname;//"logger";
	std::string logFile = logfile;//"vss.log";


	auto newLogger = spdlog::daily_logger_mt(logName, logFile, 0, 0, false, max_days);

	/* [][%@,%!] */
	spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%s:%#,%!] [%l] %v");

	/* 设置日志等级 */
	spdlog::set_level(spdlog::level::trace);

	/* 当遇到warn消息级别以上的立刻刷新到日志 */
    newLogger->flush_on(spdlog::level::info);

    /* 保存 logger */
    handle->loggers[logName] = newLogger;
    
    /* 如果是第一个 logger，设为默认 */
    if (!handle->defaultLogger) {
        handle->defaultLogger = newLogger;
    }

	return 0;
}

int initSdkLogBySize(char *logname, char *logfile, int max_file_size, int max_files)
{
    if (g_dlogInnerHandle == NULL)
    {
        g_dlogInnerHandle = new dlogInnerHandle_S;
        ((dlogInnerHandle_S*)g_dlogInnerHandle)->bSynPrintf = false;
    }
    dlogInnerHandle_S* handle = (dlogInnerHandle_S*)g_dlogInnerHandle;

	std::string logName = logname;//"logger";
	std::string logFile = logfile;//"vss.log";


	auto newLogger = spdlog::rotating_logger_mt(logName, logFile, max_file_size, max_files);

    /* [][%@,%!] */
	spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%s:%#,%!] [%l] %v");

	/* 设置日志等级 */
	spdlog::set_level(spdlog::level::trace);

	/* 当遇到warn消息级别以上的立刻刷新到日志 */
    newLogger->flush_on(spdlog::level::info);

    /* 保存 logger */
    handle->loggers[logName] = newLogger;

    /* 如果是第一个 logger，设为默认 */
    if (!handle->defaultLogger) {
        handle->defaultLogger = newLogger;
    }

	return 0;
}

int uninitSdkLog()
{
	/* 首先判断基础日志创建了没有 */
	if(g_dlogInnerHandle == NULL)
	{
		printf("no init log base!!!!\n");
		return -1;
	}

    // Release and close all loggers
    spdlog::drop_all();

    dlogInnerHandle_S* handle = (dlogInnerHandle_S*)g_dlogInnerHandle;
    delete handle;
    g_dlogInnerHandle = NULL;

	return 0;
}

int setLogLevel(int level)
{
	/* 首先判断基础日志创建了没有 */
	if(g_dlogInnerHandle == NULL)
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
    
    spdlog::set_level(spdLevel); // Global set
    
    // Set for all registered loggers explicitly just in case
    dlogInnerHandle_S* handle = (dlogInnerHandle_S*)g_dlogInnerHandle;
    for (auto& pair : handle->loggers) {
        if (pair.second) {
            pair.second->set_level(spdLevel);
        }
    }
	return 0;
}

static int createLevelLog(dlogInnerHandle_S *handle, char *logname, char *logfile, int level)
{
	if(handle == NULL)
	{
		printf("this argument is NULL!!!\n");
		return -1;
	}

	int max_days = 7;
	std::string logName = logname;//"logger";
	std::string logFile = logfile;//"vss.log";

	switch(level)
	{
		case NETSDK_LOG_WARN:
		{
            auto logger = spdlog::daily_logger_mt(logName, logFile, 0, 0, false, max_days);
            logger->set_pattern("[%H:%M:%S.%e] [%s:%#,%!] [%l] %v");
			handle->loggers[logName] = logger;
            // Also register with a suffix if needed, or rely on distinct logName
			break;
		}
		case NETSDK_LOG_ERROR:
		{
            auto logger = spdlog::daily_logger_mt(logName, logFile, 0, 0, false, max_days);
            logger->set_pattern("[%H:%M:%S.%e] [%s:%#,%!] [%l] %v");
			handle->loggers[logName] = logger;
			break;
		}
		default:
			printf("this level[%d] is not support!!!\n",level);
			return -1;
	}

    /* [][%@,%!] *//*单独设置每个日志级别的输出格式*/
//    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] %@ %! [%l] %v");

	return 0;
}

static int createLevelLogBySize(dlogInnerHandle_S* handle, char* logname, char* logfile, int level, int max_file_size, int max_files)
{
	if(handle == NULL)
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
			handle->loggers[logName] = logger;
			break;
		}
		case NETSDK_LOG_ERROR:
		{
            auto logger = spdlog::rotating_logger_mt(logName, logFile, max_file_size, max_files);
            logger->set_pattern("[%H:%M:%S.%e] [%s:%#,%!] [%l] %v");
			handle->loggers[logName] = logger;
			break;
		}
		default:
			printf("this level[%d] is not support!!!\n",level);
			return -1;
	}
	return 0;
}

int initSdkLogLevel(char *logname, char *logfile, int level)
{
	/* 首先判断基础日志创建了没有 */
	if(g_dlogInnerHandle == NULL)
	{
		printf("no init log base!!!!\n");
		return -1;
	}

	/* 再创建对应等级的日志文件 */
	int ret = 0;
	ret = createLevelLog((dlogInnerHandle_S*)g_dlogInnerHandle,\
							logname,logfile,level);
	return ret;
}

int initSdkLogLevelBySize(char *logname, char *logfile, int level, int max_file_size, int max_files)
{
	/* 首先判断基础日志创建了没有 */
    if (g_dlogInnerHandle == NULL)
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
	ret = createLevelLogBySize((dlogInnerHandle_S *) g_dlogInnerHandle, logname, logfile, level, max_file_size, max_files);
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

int netSdk_log(int level,\
		const char *filename_in, int line_in, const char *funcname_in,\
		const char *format, ...)
{
	dlogInnerHandle_S* logHandle = NULL;
	logHandle = (dlogInnerHandle_S*)g_dlogInnerHandle;

    std::string strFileName  = filename_in;

	if(logHandle == NULL)
	{
		//return 0;
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
		// int nLen = 0;
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

        /* 选择 Logger */
        std::shared_ptr<spdlog::logger> targetLogger = logHandle->defaultLogger;
        
        // 简单启发式：根据文件名判断归属
        // Server 文件通常包含 sdk_server
        // Client 文件通常包含 sdk_client
        // 注意：Windows路径可能是反斜杠，Linux是正斜杠，这里做简单子串查找
        // 将文件名统一转为小写或者直接查找（假设文件名不区分大小写或者就是小写）
        // 这里简化处理，直接查找
        std::string lowerFileName = strFileName;
//        std::transform(lowerFileName.begin(), lowerFileName.end(), lowerFileName.begin(), ::tolower); // 需要 <algorithm>

        if (strFileName.find("sdk_server") != std::string::npos || strFileName.find("SDK_SERVER") != std::string::npos) {
            auto it = logHandle->loggers.find("NetTVSDKServer");
            if (it != logHandle->loggers.end()) {
                targetLogger = it->second;
            }
        } 
        else if (strFileName.find("sdk_client") != std::string::npos || strFileName.find("SDK_CLIENT") != std::string::npos) {
            auto it = logHandle->loggers.find("NetTVSDKClient");
            if (it != logHandle->loggers.end()) {
                targetLogger = it->second;
            }
        }

        if (targetLogger) {
            switch(level)
            {
            case NETSDK_LOG_TRACE:
                targetLogger->log(spdlog::source_loc{strFileName.c_str(), line_in, funcname_in}, spdlog::level::trace, ptrFmt);
                break;
            case NETSDK_LOG_DEBUG:
                targetLogger->log(spdlog::source_loc{strFileName.c_str(), line_in, funcname_in}, spdlog::level::debug, ptrFmt);
                break;
            case NETSDK_LOG_INFO:
                targetLogger->log(spdlog::source_loc{strFileName.c_str(), line_in, funcname_in}, spdlog::level::info, ptrFmt);
                break;
            case NETSDK_LOG_WARN:
                targetLogger->log(spdlog::source_loc{strFileName.c_str(), line_in, funcname_in}, spdlog::level::warn, ptrFmt);
                break;
            case NETSDK_LOG_ERROR:
                targetLogger->log(spdlog::source_loc{strFileName.c_str(), line_in, funcname_in}, spdlog::level::err, ptrFmt);
                break;
            }
			// 手动刷新确保日志写入文件
            targetLogger->flush();
        }


#ifndef RELEASE_VERSION
		/*Debug版默认同步输出打印*/
		if (logHandle != NULL)
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
		if (logHandle != NULL && logHandle->bSynPrintf)
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

int syncPrintf(bool bStatus)
{
    dlogInnerHandle_S* logHandle = NULL;
    logHandle = (dlogInnerHandle_S*)g_dlogInnerHandle;
    if(logHandle == NULL){
        printf("no init log base!!!!\n");
        return -1;
    }
    logHandle->bSynPrintf = bStatus;
    return 0;
}

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
int setFlushLevel(int nLevel)
{
    /* 首先判断基础日志创建了没有 */
    if(g_dlogInnerHandle == NULL)
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


    dlogInnerHandle_S* handle = (dlogInnerHandle_S*)g_dlogInnerHandle;
    
    for (auto& pair : handle->loggers) {
        if (pair.second) {
            pair.second->flush_on(enLevel);
        }
    }

    return 0;
}
