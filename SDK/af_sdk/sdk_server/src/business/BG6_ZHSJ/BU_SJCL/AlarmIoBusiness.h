/*
 * 文件名：AlarmIoBusiness.h
 * 作者：ITC
 * 日期：2026-10-10
 * 功能：报警输入输出设置业务。
 * 修改记录：2026-10-10，新增列表 JSON 到 IPC 单路设置回调的适配。
 */
#pragma once

#include <string>

#include "NetTVSDKServerInterface.h"

/**
 * 报警输入输出设置业务处理类。
 * 说明：SDK 只负责将列表或单路 JSON 转换为 SDK 结构并逐路调用 IPC 回调，
 *       参数范围和设备业务规则由 IPC TVSDK 回调统一校验。
 */
class CAlarmIoBusiness
{
public:
    /**
     * 功能：设置报警输入配置，兼容单路对象和列表对象。
     * param [in] nChannelId：设备通道标识。
     * param [in] nCommand：报警输入设置命令。
     * param [in] strRequest：单路或列表 JSON。
     * param [in] strUrlParam：URL 参数。
     * param [out]：无。
     * return：设置结果响应 JSON。
     */
    static std::string HandleSetAlarmInputInfo(INT32 nChannelId, INT32 nCommand,
                                               const std::string& strRequest,
                                               const std::string& strUrlParam);

    /**
     * 功能：设置报警输出配置，兼容单路对象和列表对象。
     * param [in] nChannelId：设备通道标识。
     * param [in] nCommand：报警输出设置命令。
     * param [in] strRequest：单路或列表 JSON。
     * param [in] strUrlParam：URL 参数。
     * param [out]：无。
     * return：设置结果响应 JSON。
     */
    static std::string HandleSetAlarmOutputInfo(INT32 nChannelId, INT32 nCommand,
                                                const std::string& strRequest,
                                                const std::string& strUrlParam);
};
