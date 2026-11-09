/**
 * @FilePath     : classroom_metric_log.h
 * @Author       : leiyy
 * @Date         : 2026-10-21 16:18:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-21 16:48:00
 * @Description  : 课堂心跳的落点
 */

#pragma once

#include <cstdint>
#include <string>

#include "classroom_define.h"

namespace Classroom
{
/* 落点默认值。文件名跟着 stream.log 走（同目录、同前缀），这样按目录取日志的人
   与按 stream*.log 打包的脚本都不会漏掉它 */
#define CLASSROOM_METRIC_LOGGER_NAME   "stream_metric"
#define CLASSROOM_METRIC_FILE          "/opt/course/log/stream/stream_metric.log"
#define CLASSROOM_METRIC_MAX_KB        (4096)    /* 单份上限 */
#define CLASSROOM_METRIC_MAX_FILES     (6)       /* spdlog 留 6 份轮转件 + 当前 = 7 份 28MB */
#define CLASSROOM_METRIC_HEAD_MS       (300000)  /* 回抄进业务日志的紧凑头周期，0 = 不回抄 */
/* 装配期什么都还没跑，只能按实测稳态行长估 */
#define CLASSROOM_METRIC_EST_KB_PER_H  (920)
/* 整只心跳环的预算上限*/
#define CLASSROOM_METRIC_RING_MAX_KB   (128 * 1024)

/* 由引擎从 CEgressConf_S 拼好传进来：本类不认识配置解析，只认识参数 */
typedef struct _MetricLogParam_
{
    bool        bOwnFile      = true;
    std::string strFile       = CLASSROOM_METRIC_FILE;
    int32_t     nMaxKb        = CLASSROOM_METRIC_MAX_KB;
    int32_t     nMaxFiles     = CLASSROOM_METRIC_MAX_FILES;
    int32_t     nHeadPeriodMs = CLASSROOM_METRIC_HEAD_MS;
} CMetricLogParam_S;

/**
 * @brief 心跳落点，与它自己的读数
 */
class CClassroomMetricLog
{
public:
    CClassroomMetricLog() = default;

    /**
     * @brief 按参数决定落点，并把结果写成一句能直接进日志的话
     * @return  {const char*} 装配结果（含"为什么回落到业务日志"）。本函数绝不抛：
     *          spdlog 打不开文件是从 sink 构造函数抛异常，那一档必须以"少一个文件"收场，
     *          不能把整机算法启动一起带走（同 gateway 对 engine init 失败的处置口径）
     */
    const char *configure(const CMetricLogParam_S &stParam);

    /**
     * @brief 落一行完整心跳；走自己的文件时按 head 周期回抄一行紧凑头进业务日志
     * @param  {const std::string&} strLine：整行心跳，原样落，本函数不加工
     * @param  {const std::string&} strHead：紧凑头，只含标量列，且必须是 strLine 的前缀
     * @param  {int64_t} llNowMs：单调时钟，用于 head 周期判定与留存实测
     */
    void write(const std::string &strLine, const std::string &strHead, int64_t llNowMs);

    /* 心跳里 mlog= 那一段：落点、累计行数与字节、实测速率、按此速率还能回溯多久、回落几行。
       留存是这次改动的全部理由，所以它必须是一个**读数**，不是文档里的一句话。
       口径：本函数在落这一行**之前**被调，所以 lines= 不含正在写的这一条（第一条心跳里
       就是 lines=0、keep=warming）。这是必然的——串要先定稿才能落盘，别为了看着顺眼去 +1 */
    std::string statLine(int64_t llNowMs) const;

    /* 装配结果那句话，存下来随时可复读（启动期日志与 statusJson 都用它） */
    const std::string &how() const { return m_strHow; }

private:
    /* 落点文件还在不在。见头文件第 2 条：删掉之后写入不报错 */
    bool sinkAlive() const;

private:
    /* 几何三项跟着**进程级那只 sink** 走（建成时定，之后只读）：statLine 的 keep= 是按它算的，
       所以它必须是"实际在用的那份"，不是"上一次要求的那份" */
    CMetricLogParam_S m_stParam;
    CMetricLogParam_S m_stAsked;         /* 上一次下发要求的那一份，只用来判"这次到底变了没有" */
    bool              m_bAsked = false;  /* m_stAsked 有效。装配失败时清回去，好让下次重试 */
    bool              m_bWantOwn = false;    /* 配置要求走自己的文件 */
    bool              m_bOwn = false;        /* 而且此刻真的在往它写 */
    bool              m_bGeoStale = false;   /* 要求的几何 != 在用的几何，重启才换 */
    std::string       m_strHow = "mlog=unconfigured";

    uint64_t          m_u64Lines = 0;        /* 本进程写出的心跳行数（两条路合计） */
    uint64_t          m_u64Bytes = 0;        /* 同上，整行字节之和（不含回抄的 head） */
    uint64_t          m_u64Fallback = 0;     /* 本该走自己的文件而没能走成的行数 */
    uint64_t          m_u64Head = 0;         /* 回抄进业务日志的紧凑头行数 */
    int64_t           m_llFirstMs = 0;       /* 第一行心跳的时刻，速率与留存的基准 */
    int64_t           m_llLastHeadMs = 0;
    int64_t           m_llLastWarnMs = 0;    /* 落点异常那条 warn 的限速窗 */
};

}
