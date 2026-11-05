/*
 * @FilePath     : sdk_new/sdk_server/src/business/BG6_ZHSJ/BU_SJCL/SjclDomain.h
 * @Author       : chenchl
 * @Date         : 2026-08-20
 * @LastEditors  : chenchl
 * @LastEditTime : 2026-08-20
 * @Description  : SJCL 配置域（NVR/录播，条件编译）
 *                 收口 NVR/录播设备相关命令：录像状态/计划/高级参数、录像文件查询、
 *                 通道信息、RTSP URL、录像控制/下载。
 *                 编译宏：未定义 BU_SJCL_EXCLUDE 时编入，按 BG 群编译时可通过宏排除。
 *                 委托 CNvrBusiness 处理 NVR 专属命令（通道信息/RTSP/录像文件列表）。
 */
#ifndef BU_SJCL_EXCLUDE

#pragma once

#include "ConfigDomainBase.h"
#include "Singleton.h"
#include "UrlParamUtils.h"

#include <cstring>
#include <string>

/**
 * SJCL 配置域
 * @details 单例，注册 NVR/录播相关命令，查表分发。
 *          条件编译：未定义 BU_SJCL_EXCLUDE 时编入。
 */
class CSjclDomain : public CConfigDomainBase, public CSingleton<CSjclDomain>
{
    friend class CSingleton<CSjclDomain>;
    CSjclDomain();
public:
    ~CSjclDomain() {}

    /* 前后智能事件专用 Get（注册时显式选用）：URL 参数 Mode=0/1 指定读取方向，未携带(-1)按本地存储模式（原行为） */
    template<typename T_CFG>
    static std::string TemplatedGetSmart(INT32 nChannelId, INT32 nCommand,
                                         const std::string& req_data,
                                         const std::string& url_param)
    {
        (void)req_data; /* 前后智能事件不带请求体参数 */
        T_CFG stCfg;
        memset(&stCfg, 0, sizeof(T_CFG));
        stCfg.nMode = ParseIntParam(url_param, "Mode", -1); /* Mode: 0=前智能 1=后智能 缺省按本地模式 */

        NETSDK_LOG_MESSAGE_INFO("GetDevConfig callback START");
        int nRespCode = executeGetDevConfigCb(nChannelId, nCommand, &stCfg);
        if (nRespCode != NET_E_SUCCEED)
        {
            NETSDK_LOG_MESSAGE_WARN("GetDevConfig callback failed, cmd=%d, ret=%d", nCommand, nRespCode);
        }
        NETSDK_LOG_MESSAGE_INFO("GetDevConfig callback cmd=%d, ret=%d", nCommand, nRespCode);
        return SDKConvert::to_respString(nRespCode, nCommand, nChannelId, stCfg);
    }
};

#endif /* BU_SJCL_EXCLUDE */
