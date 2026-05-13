/*** 
 * @FilePath     : log_handler.h
 * @Author       : zhangjc (zhangjc@kfb.cn)
 * @Date         : 2025-02-11 09:59:24
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-08-05 20:00:44
 * @Description  : 日志处理类
 */

#pragma once

#include "log_database.h"
#include "Singleton.h"

class LogHandler : public CSingleton<LogHandler>
{
    LogHandler() = default;
public:
    ~LogHandler() = default;
    friend class CSingleton<LogHandler>;

    void write(Log::Info_S stInfo);
    int find(Log::RetrievalCond_S &stRetrievalCond, Common::PageInfo_S &stPageInfo, std::vector<Log::Info_S> &logInfos);
private:
    /* 获取当前日期和时间 */
    std::string get_dateTime();
};
