/*
 * @FilePath     : sdk_new/sdk_server/src/business/BG6_ZHSJ/BU_SJLB/SjlbDomain.h
 * @Author       : chenchl
 * @Date         : 2026-08-22
 * @LastEditors  : chenchl
 * @LastEditTime : 2026-08-22
 * @Description  : BU_SJLB 配置域（设备基础，条件编译）
 *                 收口录播部门独有命令：录制控制/状态、直播控制/状态。
 *                 编译宏：未定义 BU_SJLB_EXCLUDE 时编入，按 BG 群编译时可通过宏排除。
 */
#ifndef BU_SJLB_EXCLUDE

#pragma once

#include "ConfigDomainBase.h"
#include "Singleton.h"

/**
 * BU_SJLB 配置域
 * @details 单例，注册录制/直播相关命令，查表分发。
 *          条件编译：未定义 BU_SJLB_EXCLUDE 时编入。
 */
class CBujlbDomain : public CConfigDomainBase, public CSingleton<CBujlbDomain>
{
    friend class CSingleton<CBujlbDomain>;
    CBujlbDomain();
public:
    ~CBujlbDomain() {}

    /**
     * @brief 获取录制文件列表（自定义处理）
     * @details 从请求 JSON 解析分页参数，调用设备回调填充文件列表
     */
    static std::string HandleGetRecordFileList(INT32 nChannelId, INT32 nCommand,
                                              const std::string& req_data,
                                              const std::string& url_param);

    /**
    * @brief 判断命令码是否为 BU_SJLB 域的设备级命令（不需要通道号）
    * @details 录播独有命令（录制/直播/文件列表/导播/云台/预置位/中控/布局/
    *          PVW2PGM/预约录制）均为设备级，集中在本域维护；
    *          公用命令（系统信息/注册/重启/升级/网络/音频/SSH）不在此列表。
    */
    virtual bool IsDeviceLevelCommand(INT32 nCommand) const override;


    /**
     * @brief 获取布局信息（自定义处理）
     * @details 从请求 JSON 解析 MovieMode/MPlayout，调用设备回调填充布局数据
     */
    static std::string HandleGetLayout(INT32 nChannelId, INT32 nCommand,
                                       const std::string& req_data,
                                       const std::string& url_param);
};

#endif /* BU_SJLB_EXCLUDE */
