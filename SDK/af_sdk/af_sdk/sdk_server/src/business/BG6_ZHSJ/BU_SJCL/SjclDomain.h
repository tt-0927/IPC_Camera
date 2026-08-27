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
};

#endif /* BU_SJCL_EXCLUDE */
