/*
 * @Author       : EasonLu
 * @Date         : 2025-02-12 17:55:38
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-03-21 11:06:01
 * @FilePath     : ModuleLog.cpp
 * @Description  : 模块化日志
 */
#include "ModuleLog.h"
#include <cstring>
#include <ctime>
#include <iostream>
#include <stdarg.h>
#include <string>
#include <sys/time.h>
#include <vector>

/* 日志回调 */
ModuleLog g_pModuleLog = nullptr;
/* 调试输出标记位 */
int g_bModuleLogPrint = 0;

void set_module_log(ModuleLog pModuleLog)
{
    g_pModuleLog = pModuleLog;
}

void enable_module_log_print(int bEnable)
{
    g_bModuleLogPrint = bEnable;
}

void module_log(
    int nLevel,
    const char *pFilename, int nLine, const char *pFuncname,
    const char *pFormat, ...)
{
    va_list args0, args1;
    va_start(args0, pFormat);
    va_copy(args1, args0);
    size_t nDataSize = std::vsnprintf(nullptr, 0, pFormat, args0);
    va_end(args0);

    std::vector<char> buffer(nDataSize + 1);
    std::vsnprintf(buffer.data(), buffer.size(), pFormat, args1);
    va_end(args1);

    /* 组装完整的日志记录 */
    std::string strLog;
    std::string strTime;
    std::string strLevel;
    std::string strFileLine;

    { /* 组装时间 */
        /* 获取当前时间（包括毫秒） */
        struct timeval tv;
        gettimeofday(&tv, nullptr);

        /* 转换为本地时间 */
        time_t t = tv.tv_sec;
        std::tm *ptm = std::localtime(&t);

        /* 格式化时间字符串 */
        char achFormatTime[32];
        strftime(achFormatTime, sizeof(achFormatTime), "%Y-%m-%d %H:%M:%S", ptm);

        /* 获取毫秒部分 */
        int millis = tv.tv_usec / 1000;

        /* 组装最终的时间字符串 */
        char ahcMillTime[32];
        snprintf(ahcMillTime, sizeof(ahcMillTime), ".%03d", millis);

        strTime += "[";
        strTime += achFormatTime;
        strTime += ahcMillTime;
        strTime += "]";
    }

    { /* 组装等级前缀 */

        switch (nLevel)
        {
        case MODULE_LOG_TRACE:
            strLevel += "[TRACE]";
            break;
        case MODULE_LOG_DEBUG:
            strLevel += "[DEBUG]";
            break;
        case MODULE_LOG_INFO:
            strLevel += "[INFO]";
            break;
        case MODULE_LOG_WARN:
            strLevel += "[WARN]";
            break;
        case MODULE_LOG_ERROR:
            strLevel += "[ERROR]";
            break;
        default:
            strLevel += "[UNKNOWN]";
            break;
        }
    }

    { /* 组装文件名称和行号 */
        /* 截取绝对路径中的文件名称 */
        std::string strFileName = pFilename;
        size_t nPos = strFileName.find_last_of("/\\");
        if (nPos != std::string::npos)
        {
            strFileName = strFileName.substr(nPos + 1);
        }

        strFileLine += "[";
        strFileLine += strFileName;
        strFileLine += ":";
        strFileLine += std::to_string(nLine);
        strFileLine += "]";
    }

    /* NOTE 输出到终端 */
    if (g_bModuleLogPrint)
    {
        /* 组装内容 */
        strLog += strTime;
        strLog += strLevel;
        strLog += strFileLine;
        strLog += buffer.data();
        std::cout << strLog << std::endl;
    }
    /* 上抛给回调 */
    if (g_pModuleLog)
    {
        auto strUpload = strFileLine + buffer.data();
        g_pModuleLog(nLevel, strUpload.c_str());
    }
}