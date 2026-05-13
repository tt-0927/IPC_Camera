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
#include <iostream>
#include <string>
#include <time.h>
#include "dlog.h"
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
// #include "mqtt_ctrl_communication.h"

typedef struct dlogHandle
{
	/* 基础日志，该日志是必须创建的，
	 * 其他等级若没有创建对应的日志文件，
	 * 则都写道该日志文件中
	 */
    std::shared_ptr<spdlog::logger> logger;

    /* 这里可拓展不同的日志等级输出到不同的文件中 */
    /* 错误日志 */
    std::shared_ptr<spdlog::logger> errorLogger;
    /* 警告日志 */
    std::shared_ptr<spdlog::logger> warnLogger;
    /* 用户日志，记录用户操作信息 */
    std::shared_ptr<spdlog::logger> userLogger;
	/* 用户日志，记录广播屏操作信息 */
    std::shared_ptr<spdlog::logger> dacuLogger;
	/* 故障日志：系统故障日志（中文） */
	std::shared_ptr<spdlog::logger> faultLogger;

    bool bSynPrintf;
}dlogInnerHandle_S;

dlogHandle_S g_dlogInnerHandle = NULL;

int initLog(char* logname,char* logfile)
{
	dlogInnerHandle_S* handle = new dlogInnerHandle_S;
	if(handle == NULL)
	{
		printf("malloc error!!!!\n");
		return -1;
	}

	int max_days = 7;
	std::string logName = logname;//"logger";
	std::string logFile = logfile;//"vss.log";

	/*默认不同步输出打印*/
	handle->bSynPrintf = false;

	handle->logger = spdlog::daily_logger_mt(logName, logFile, 0, 0, false, max_days);

	/* [][%@,%!] */
	spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%s:%#,%!] [%l] %v");

	/* 设置日志等级 */
	spdlog::set_level(spdlog::level::trace);

	/* 当遇到warn消息级别以上的立刻刷新到日志，
	 * 也可设置为遇到warning级别及其以上的立刻刷新
	 */
	handle->logger->flush_on(spdlog::level::warn);

    /*每2秒刷新一次*/
    // spdlog::flush_every(std::chrono::seconds(2));

	g_dlogInnerHandle = handle;
	return 0;
}

int initLogBySize(char *logname, char *logfile, int max_file_size, int max_files)
{
	dlogInnerHandle_S* handle = new dlogInnerHandle_S;
	if(handle == NULL)
	{
		printf("malloc error!!!!\n");
		return -1;
	}

	std::string logName = logname;//"logger";
	std::string logFile = logfile;//"vss.log";

	/*默认不同步输出打印*/
	handle->bSynPrintf = false;

	handle->logger = spdlog::rotating_logger_mt(logName, logFile, max_file_size, max_files);

    /* [][%@,%!] */
	spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%s:%#,%!] [%l] %v");

	/* 设置日志等级 */
	spdlog::set_level(spdlog::level::trace);

	/* 当遇到warn消息级别以上的立刻刷新到日志，
	 * 也可设置为遇到warning级别及其以上的立刻刷新
	 */
	handle->logger->flush_on(spdlog::level::warn);

    /*每2秒刷新一次*/
    // spdlog::flush_every(std::chrono::seconds(2));

	g_dlogInnerHandle = handle;
	return 0;
}

int uninitLog()
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
	switch(level)
	{
		case LOG_TRACE:
		{
			spdlog::set_level(spdlog::level::trace);
			break;
		}
		case LOG_DEBUG:
		{
			spdlog::set_level(spdlog::level::debug);
			break;
		}
		case LOG_INFO:
		{
			spdlog::set_level(spdlog::level::info);
			break;
		}
		case LOG_WARN:
		{
			spdlog::set_level(spdlog::level::warn);
			break;
		}
		case LOG_ERROR:
		{
			spdlog::set_level(spdlog::level::err);
			break;
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
		case LOG_FAULT:
		{
			handle->faultLogger = spdlog::daily_logger_mt(\
					logName, logFile, 0, 0, false, max_days);
            handle->faultLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
			break;
		}
		case LOG_USER:
		{
			handle->userLogger = spdlog::daily_logger_mt(\
					logName, logFile, 0, 0, false, max_days);
            handle->userLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
			break;
		}
		case LOG_WARN:
		{
			handle->warnLogger = spdlog::daily_logger_mt(\
					logName, logFile, 0, 0, false, max_days);
            handle->warnLogger->set_pattern("[%H:%M:%S.%e] [%s:%#,%!] [%l] %v");
			break;
		}
		case LOG_ERROR:
		{
			handle->errorLogger = spdlog::daily_logger_mt(\
					logName, logFile, 0, 0, false, max_days);
            handle->errorLogger->set_pattern("[%H:%M:%S.%e] [%s:%#,%!] [%l] %v");
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
		case LOG_FAULT:
		{
			handle->faultLogger = spdlog::rotating_logger_mt(logName, logFile, max_file_size, max_files);
            handle->faultLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
			break;
		}
		case LOG_USER:
		{
			handle->userLogger = spdlog::rotating_logger_mt(logName, logFile, max_file_size, max_files);
            handle->userLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
			break;
		}
		case LOG_WARN:
		{
			handle->warnLogger = spdlog::rotating_logger_mt(logName, logFile, max_file_size, max_files);
            handle->warnLogger->set_pattern("[%H:%M:%S.%e] [%s:%#,%!] [%l] %v");
			break;
		}
		case LOG_ERROR:
		{
			handle->errorLogger = spdlog::rotating_logger_mt(logName, logFile, max_file_size, max_files);
            handle->errorLogger->set_pattern("[%H:%M:%S.%e] [%s:%#,%!] [%l] %v");
			break;
		}
		default:
			printf("this level[%d] is not support!!!\n",level);
			return -1;
	}
	return 0;
}

int initLogLevel(char *logname, char *logfile, int level)
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

int initLogLevelBySize(char *logname, char *logfile, int level, int max_file_size, int max_files)
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

int dlog_printf(int level,\
		const char *filename_in, int line_in, const char *funcname_in,\
		const char *format, ...)
{
	dlogInnerHandle_S* logHandle = NULL;
	logHandle = (dlogInnerHandle_S*)g_dlogInnerHandle;

    std::string strFileName  = filename_in;

	if(logHandle == NULL)
	{
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
			// printf("%s\n", sendMsg);
			// nLen = strlen(sendMsg);
		}

		va_end(args1);

		std::shared_ptr<spdlog::logger> logger;
		switch(level)
		{
		case LOG_TRACE:
		{
			(logHandle->logger)->log(spdlog::source_loc{\
				strFileName.c_str(), line_in, funcname_in}, \
					spdlog::level::trace, ptrFmt);
			break;
		}
		case LOG_DEBUG:
		{
			(logHandle->logger)->log(spdlog::source_loc{\
				strFileName.c_str(), line_in, funcname_in}, \
					spdlog::level::debug, ptrFmt);
			break;
		}
		case LOG_INFO:
		{
			(logHandle->logger)->log(spdlog::source_loc{\
				strFileName.c_str(), line_in, funcname_in}, \
					spdlog::level::info, ptrFmt);

			// if (nLen != 0)
			// 	publish_msg(MQTT_TYPE_OPERATION, MQTT_LEVEL_DEFAULT, MQTT_SOURCE_FUNCTIONAL_OPERATION, sendMsg, nLen);
			break;
		}
		case LOG_WARN:
		{
			logger = (logHandle->warnLogger) ? logHandle->warnLogger : \
					logHandle->logger;
			logger->log(spdlog::source_loc{\
				strFileName.c_str(), line_in, funcname_in}, \
					spdlog::level::warn, ptrFmt);

			// if (nLen != 0)
			// 	publish_msg(MQTT_TYPE_EXCEPTION, MQTT_LEVEL_SLIGHT, MQTT_SOURCE_PROGRAM_ERROR, sendMsg, nLen);
			break;
		}
		case LOG_ERROR:
		{
			logger = (logHandle->errorLogger) ? logHandle->errorLogger : \
					logHandle->logger;
			logger->log(spdlog::source_loc{\
				strFileName.c_str(), line_in, funcname_in}, \
					spdlog::level::err, ptrFmt);

			// if (nLen != 0)
			// 	publish_msg(MQTT_TYPE_EXCEPTION, MQTT_LEVEL_SERIOUS, MQTT_SOURCE_PROGRAM_ERROR, sendMsg, nLen);
			break;
		}
		case LOG_USER:
		{
			logger = (logHandle->userLogger) ? logHandle->userLogger : \
					logHandle->logger;
			logger->log(spdlog::source_loc{\
				NULL, 0, NULL}, \
					spdlog::level::info, ptrFmt);
			break;
		}
		case LOG_FAULT:
		{
			logger = (logHandle->faultLogger) ? logHandle->faultLogger : \
					logHandle->logger;
			logger->log(spdlog::source_loc{\
				NULL, 0, NULL}, \
					spdlog::level::info, ptrFmt);

			// if (nLen != 0)
			// 	publish_msg(MQTT_TYPE_EXCEPTION, MQTT_LEVEL_DEADLY, MQTT_SOURCE_PROGRAM_ERROR, sendMsg, nLen);
			break;
		}
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
    case LOG_TRACE:enLevel = spdlog::level::trace;break;
    case LOG_DEBUG:enLevel = spdlog::level::debug;break;
    case LOG_INFO:enLevel = spdlog::level::info;break;
    default:break;
    }


    dlogInnerHandle_S* handle = (dlogInnerHandle_S*)g_dlogInnerHandle;
    handle->logger->flush_on(enLevel);

    return 0;
}
