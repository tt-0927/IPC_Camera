/*
 * @Author: leiyy leiyy@kfb.cn
 * @Date: 2026-06-15 09:19:37
 * @LastEditors: leiyy leiyy@kfb.cn
 * @LastEditTime: 2026-06-26 14:37:24
 * @FilePath: /RV1126B/share/ipc_share/control/business/log/log_handler.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
/*** 
 * @FilePath     : log_handler.h
 * @Author       : zhangjc (zhangjc@kfb.cn)
 * @Date         : 2025-02-11 09:59:24
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-08-05 20:00:44
 * @Description  : 日志处理类
 */

#pragma once

#include <atomic>
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

    /**
     * @brief 初始化日志上传开关（从配置文件读取）
     */
    void initLogUpload();
    /**
     * @brief 设置日志上传开关（实时生效）
     * @param bEnable true=开启上传, false=关闭上传
     */
    void setLogUpload(bool bEnable);
    /**
     * @brief 获取日志上传开关状态
     */
    bool getLogUpload() const { return m_bLogUpload; }

private:
    /* 获取当前日期和时间 */
    std::string get_dateTime();

    /* 日志上传开关（缓存，避免每次write都读文件） */
    std::atomic<bool> m_bLogUpload{false};
};
