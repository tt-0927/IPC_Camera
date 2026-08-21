/**
 * @FilePath     : dlog.cpp
 * @Author       : zhangjunbin
 * @Date         : 2021年3月30日
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-08 10:20:09
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
#include <chrono>
#include <fstream>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <atomic>
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

/* ---- 日志限流 ---- */
/* 限流间隔（毫秒），0 表示禁用限流 */
static int g_nLogThrottleIntervalMs = 0;
/* 限流互斥锁，保护限流 Map */
static std::mutex g_mtxThrottle;

struct LogThrottleEntry_S
{
    /* 上次输出的时间点 */
    std::chrono::steady_clock::time_point tpLastTime;
};
/* key: (filename_ptr << 32) | line，利用文件字符串地址+行号唯一标识调用点 */
static std::unordered_map<size_t, LogThrottleEntry_S> g_mapThrottle;
/* 限流 Map 最大容量，超过时清空重建，防止内存持续增长 */
static constexpr size_t THROTTLE_MAP_MAX_SIZE = 512;

/* ---- 日志级别文件监控 ---- */
/* 监控线程运行标志 */
static std::atomic<bool> g_bMonitorRunning(false);
/* 监控线程句柄，用于进程退出时 join */
static std::shared_ptr<std::thread> g_pMonitorThread;
/* 日志级别文件路径，setLogLevel 时同步写入，监控线程读取 */
static std::string g_strLevelFilePath;
/* 日志级别文件路径的互斥锁 */
static std::mutex g_mtxLevelFile;

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

	/* 异步模式：I/O 在后台线程执行，不阻塞业务线程 */
	auto tp = std::make_shared<spdlog::details::thread_pool>(8192, 1);
	/* 将 thread_pool 注册到全局 registry，确保其生命周期覆盖所有 async_logger */
	spdlog::details::registry::instance().set_tp(tp);
	auto sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(logFile, 0, 0, false, max_days);
	handle->logger = std::make_shared<spdlog::async_logger>(
		logName, sink, tp, spdlog::async_overflow_policy::block);
	spdlog::details::registry::instance().initialize_logger(handle->logger);

	/* 异步 logger 必须通过实例方法设置 pattern，spdlog::set_pattern 对异步 logger 无效 */
	handle->logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%s:%#,%!] [%l] %v");

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

	/* 异步模式：I/O 在后台线程执行，不阻塞业务线程 */
	auto tp = std::make_shared<spdlog::details::thread_pool>(8192, 1);
	/* 将 thread_pool 注册到全局 registry，确保其生命周期覆盖所有 async_logger */
	spdlog::details::registry::instance().set_tp(tp);
	auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(logFile, max_file_size, max_files);
	handle->logger = std::make_shared<spdlog::async_logger>(
		logName, sink, tp, spdlog::async_overflow_policy::block);
	spdlog::details::registry::instance().initialize_logger(handle->logger);

    /* 异步 logger 必须通过实例方法设置 pattern，spdlog::set_pattern 对异步 logger 无效 */
	handle->logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%s:%#,%!] [%l] %v");

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

	/* 停止日志级别监控线程，避免 drop_all 后仍操作已销毁的 logger 导致段错误 */
	if (g_bMonitorRunning.load())
	{
		g_bMonitorRunning.store(false);
		if (g_pMonitorThread && g_pMonitorThread->joinable())
		{
			g_pMonitorThread->join();
		}
		g_pMonitorThread.reset();
	}

	/* 清理限流 Map，释放内存 */
	{
		std::lock_guard<std::mutex> lock(g_mtxThrottle);
		g_mapThrottle.clear();
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

	/* 同步写入级别文件，确保文件内容与当前日志级别一致 */
	{
		std::lock_guard<std::mutex> lock(g_mtxLevelFile);
		if (!g_strLevelFilePath.empty())
		{
			const char *pLevelStr = nullptr;
			switch (level)
			{
			case LOG_TRACE: pLevelStr = "trace"; break;
			case LOG_DEBUG: pLevelStr = "debug"; break;
			case LOG_INFO:  pLevelStr = "info";  break;
			case LOG_WARN:  pLevelStr = "warn";  break;
			case LOG_ERROR: pLevelStr = "error"; break;
			default: break;
			}
			if (pLevelStr != nullptr)
			{
				std::ofstream ofs(g_strLevelFilePath, std::ios::trunc);
				if (ofs.is_open())
				{
					ofs << pLevelStr;
					ofs.close();
				}
			}
		}
	}

	return 0;
}

/**
 * @brief   : 创建异步级别日志（内部辅助函数）
 * @note    : 所有级别日志统一使用异步模式，与主日志共享同一个 thread_pool
 */
static std::shared_ptr<spdlog::async_logger> createAsyncLevelLogger(
    const std::string &logName,
    std::shared_ptr<spdlog::sinks::sink> sink,
    const std::string &pattern)
{
    auto &registry = spdlog::details::registry::instance();
    auto tp = registry.get_tp();
    if (tp == nullptr)
    {
        /* 主日志尚未初始化，创建默认 thread_pool */
        tp = std::make_shared<spdlog::details::thread_pool>(8192, 1);
        registry.set_tp(tp);
    }
    auto logger = std::make_shared<spdlog::async_logger>(
        logName, sink, tp, spdlog::async_overflow_policy::block);
    /* 先设置 pattern，再注册到 registry，避免 registry 的全局 pattern 覆盖自定义 pattern */
    logger->set_pattern(pattern);
    registry.initialize_logger(logger);
    return logger;
}

static int createLevelLog(dlogInnerHandle_S *handle, char *logname, char *logfile, int level)
{
	if(handle == NULL)
	{
		printf("this argument is NULL!!!\n");
		return -1;
	}

	int max_days = 7;
	std::string logName = logname;
	std::string logFile = logfile;

	switch(level)
	{
		case LOG_FAULT:
		{
			auto sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(logFile, 0, 0, false, max_days);
			handle->faultLogger = createAsyncLevelLogger(logName, sink, "[%Y-%m-%d %H:%M:%S.%e] %v");
			break;
		}
		case LOG_USER:
		{
			auto sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(logFile, 0, 0, false, max_days);
			handle->userLogger = createAsyncLevelLogger(logName, sink, "[%Y-%m-%d %H:%M:%S.%e] %v");
			break;
		}
		case LOG_WARN:
		{
			auto sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(logFile, 0, 0, false, max_days);
			handle->warnLogger = createAsyncLevelLogger(logName, sink, "[%H:%M:%S.%e] [%s:%#,%!] [%l] %v");
			break;
		}
		case LOG_ERROR:
		{
			auto sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(logFile, 0, 0, false, max_days);
			handle->errorLogger = createAsyncLevelLogger(logName, sink, "[%H:%M:%S.%e] [%s:%#,%!] [%l] %v");
			break;
		}
		default:
			printf("this level[%d] is not support!!!\n",level);
			return -1;
	}

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
			auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(logFile, max_file_size, max_files);
			handle->faultLogger = createAsyncLevelLogger(logName, sink, "[%Y-%m-%d %H:%M:%S.%e] %v");
			break;
		}
		case LOG_USER:
		{
			auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(logFile, max_file_size, max_files);
			handle->userLogger = createAsyncLevelLogger(logName, sink, "[%Y-%m-%d %H:%M:%S.%e] %v");
			break;
		}
		case LOG_WARN:
		{
			auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(logFile, max_file_size, max_files);
			handle->warnLogger = createAsyncLevelLogger(logName, sink, "[%H:%M:%S.%e] [%s:%#,%!] [%l] %v");
			break;
		}
		case LOG_ERROR:
		{
			auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(logFile, max_file_size, max_files);
			handle->errorLogger = createAsyncLevelLogger(logName, sink, "[%H:%M:%S.%e] [%s:%#,%!] [%l] %v");
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
	/* 限流检查：在格式化之前拦截，同时节省 CPU 和 I/O */
	/* ERROR 和 FAULT 级别不限流，确保关键日志不被吞掉 */
	if (g_nLogThrottleIntervalMs > 0 && level < LOG_ERROR)
	{
		/* 利用文件名指针和行号构造唯一 key，同一调用点的指针地址在编译后不变 */
		size_t nKey = (reinterpret_cast<size_t>(filename_in) ^ static_cast<size_t>(line_in * 2654435761u));
		auto now = std::chrono::steady_clock::now();

		std::lock_guard<std::mutex> lock(g_mtxThrottle);
		auto it = g_mapThrottle.find(nKey);
		if (it != g_mapThrottle.end())
		{
			auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second.tpLastTime).count();
			if (elapsed < g_nLogThrottleIntervalMs)
			{
				/* 未达到间隔，直接跳过本次日志 */
				return 0;
			}
			it->second.tpLastTime = now;
		}
		else
		{
			/* Map 过大时清空重建，避免内存持续增长 */
			if (g_mapThrottle.size() >= THROTTLE_MAP_MAX_SIZE)
			{
				g_mapThrottle.clear();
			}
			g_mapThrottle[nKey] = {now};
		}
	}

	dlogInnerHandle_S* logHandle = NULL;
	logHandle = (dlogInnerHandle_S*)g_dlogInnerHandle;

    /* 仅用于当前函数内的控制台文件名处理；不得把其 c_str() 交给异步 logger。 */
    std::string strFileName = filename_in != NULL ? filename_in : "";

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
			/* source_loc 不深拷贝文件名，必须使用生命周期覆盖整个进程的 __FILE__ 指针。 */
			(logHandle->logger)->log(spdlog::source_loc{\
				filename_in, line_in, funcname_in}, \
					spdlog::level::trace, ptrFmt);
			break;
		}
		case LOG_DEBUG:
		{
			(logHandle->logger)->log(spdlog::source_loc{\
				filename_in, line_in, funcname_in}, \
					spdlog::level::debug, ptrFmt);
			break;
		}
		case LOG_INFO:
		{
			(logHandle->logger)->log(spdlog::source_loc{\
				filename_in, line_in, funcname_in}, \
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
				filename_in, line_in, funcname_in}, \
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
				filename_in, line_in, funcname_in}, \
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

        /* 同步输出到控制台：Debug 和 Release 统一由 bSynPrintf 控制 */
        /* 各进程 main 中通过 syncPrintf() 设置，CAP_PROCESS_LOG_SWITCH 控制初始值 */
        if (logHandle != NULL && logHandle->bSynPrintf)
        {
            char timeBuf[64];
            getCurrTime(timeBuf, NULL);
            size_t lastSlash = strFileName.find_last_of("/\\");
            std::string fileNameOnly = (lastSlash != std::string::npos) ? strFileName.substr(lastSlash + 1) : strFileName;
            printf("%s[%s:%d] %s\n", timeBuf, fileNameOnly.c_str(), line_in, ptrFmt);
            fflush(stdout);
        }
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

int setLogThrottleInterval(int nIntervalMs)
{
    if (nIntervalMs < 0)
    {
        return -1;
    }
    g_nLogThrottleIntervalMs = nIntervalMs;
    return 0;
}

/* 监控间隔（秒） */
static constexpr int LOG_LEVEL_CHECK_INTERVAL_SEC = 5;

/**
 * @brief   : 从文件读取日志级别字符串，转换为日志级别常量
 * @param   {const std::string&} strLevel：级别字符串（trace/debug/info/warn/error）
 * @return  {int} 对应的 LOG_xxx 常量，-1 表示无法识别
 */
static int parseLogLevelFromFile(const std::string &strLevel)
{
    /* 去除首尾空白字符 */
    std::string strTrimmed = strLevel;
    size_t nStart = strTrimmed.find_first_not_of(" \t\r\n");
    size_t nEnd = strTrimmed.find_last_not_of(" \t\r\n");
    if (nStart == std::string::npos)
    {
        return -1;
    }
    strTrimmed = strTrimmed.substr(nStart, nEnd - nStart + 1);

    /* 转为小写比较 */
    for (auto &ch : strTrimmed)
    {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }

    if (strTrimmed == "trace") return LOG_TRACE;
    if (strTrimmed == "debug") return LOG_DEBUG;
    if (strTrimmed == "info")  return LOG_INFO;
    if (strTrimmed == "warn")  return LOG_WARN;
    if (strTrimmed == "error") return LOG_ERROR;

    return -1;
}

/**
 * @brief   : 日志级别文件监控线程函数
 * @note    : 定时读取指定文件，文件内容变化时切换日志级别
 */
static void logLevelMonitorThread(const std::string strFilePath)
{
    /* 设置线程名，便于调试和性能分析时识别线程 */
    pthread_setname_np(pthread_self(), "log_lvl_mon");

    int nLastLevel = -1;

    while (g_bMonitorRunning.load())
    {
        std::ifstream ifs(strFilePath);
        if (ifs.is_open())
        {
            std::string strContent;
            std::getline(ifs, strContent);
            ifs.close();

            int nLevel = parseLogLevelFromFile(strContent);
            if (nLevel >= 0 && nLevel != nLastLevel)
            {
                setLogLevel(nLevel);
                nLastLevel = nLevel;
                /* 级别切换时立即输出一条提示，方便确认生效 */
                dlog_info("日志级别已切换为: %s", strContent.c_str());
            }
        }

        /* 分段 sleep，每 500ms 检查一次退出标志，避免退出时长时间阻塞 */
        for (int i = 0; i < LOG_LEVEL_CHECK_INTERVAL_SEC * 2 && g_bMonitorRunning.load(); i++)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
}

int startLogLevelMonitor(const char *pLevelFilePath)
{
    if (pLevelFilePath == NULL || pLevelFilePath[0] == '\0')
    {
        return -1;
    }

    /* 设置级别文件路径，使 setLogLevel 能够同步写入 */
    {
        std::lock_guard<std::mutex> lock(g_mtxLevelFile);
        g_strLevelFilePath = pLevelFilePath;
    }

    if (g_bMonitorRunning.load())
    {
        /* 已经启动过，先停止旧的监控线程 */
        g_bMonitorRunning.store(false);
        if (g_pMonitorThread && g_pMonitorThread->joinable())
        {
            g_pMonitorThread->join();
        }
    }

    g_bMonitorRunning.store(true);
    g_pMonitorThread = std::make_shared<std::thread>(logLevelMonitorThread, std::string(pLevelFilePath));

    /* 不 detach，在 uninitLog 中 join，确保进程退出前监控线程已安全停止 */
    /* 避免 detach 后监控线程操作已销毁的 spdlog 实例导致段错误 */

    return 0;
}
