/**
 * @FilePath     : osd_manage.h
 * @Author       : huangjunda
 * @Date         : 2025-05-26 15:55:28
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-28 11:15:27
 * @Description  : OSD管理类
 */

#ifndef __OSD_MANAGE_H__
#define __OSD_MANAGE_H__

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <iconv.h>
#include <mutex>
#include <net/if.h>

#include "osd_define.h"
#include "osd_configure.h"
#include "system_manage.h"
#include "os_mutex.h"
#include "Singleton.h"
#include "IpcRet.h"
#include "path_define.h"
#include "stream_vpss.h"
#include "stream_venc.h"
#include "ft2build.h"
#include "ini_disposed.h"
#include "event_define.h"

#if CAP_EXHIBITION_OSD_PANEL
#include "osd_panel/osd_panel_result.hpp"
#endif

extern "C" {
#include <sys/time.h>
#include "rockit_rgn.h"
}

#define MAX_OSD_NUM         8
#define MAX_OSD_OVERLAY_NUM 8
#define MAX_OSD_COVER_NUM   4

class COsdManage : public CSingleton<COsdManage>, public IOsdConfigApplier
{
    COsdManage();

public:
    virtual ~COsdManage();
    /* 允许 Singleton 访问私有构造函数 */
    friend class CSingleton<COsdManage>;

    /***
     * @description : 初始化
     * @author      : huangjunda
     * @return       {IpcRet_E}
     */
    IpcRet_E init();

    /***
     * @description : 去初始化
     * @author      : huangjunda
     * @return       {IpcRet_E}
     */
    IpcRet_E deinit();

    /***
     * @description : 获取osd配置
     * @author      : huangjunda
     * @param        {Osd::OsdConfig_S} &stInfo
     * @return       {IpcRet_E}
     * @note         : 转发共享层 COsdConfigure，保留旧接口兼容未迁移调用方
     */
    IpcRet_E get_osd_config(Osd::OsdConfig_S &stInfo);

    /***
     * @description : 设置osd配置
     * @author      : huangjunda
     * @param        {Osd::OsdConfig_S} stInfo
     * @return       {IpcRet_E}
     * @note         : 转发共享层 COsdConfigure，保留旧接口兼容未迁移调用方
     */
    IpcRet_E set_osd_config(Osd::OsdConfig_S stInfo);

    /***
     * @description : 获取overplay信息
     * @author      : huangjunda
     * @param        {vector<Osd::OverplayInfo_S>} &vecInfo
     * @return       {IpcRet_E}
     */
    IpcRet_E get_overplay_info(std::vector<Osd::OverplayInfo_S> &vecInfo) override;

    /***
     * @description : 设置overplay信息
     * @author      : huangjunda
     * @param        {vector<Osd::OverplayInfo_S>} vecInfo
     * @return       {IpcRet_E}
     */
    IpcRet_E set_overplay_info(std::vector<Osd::OverplayInfo_S> vecInfo);

    /**
     * @brief   : 应用overplay信息（IOsdConfigApplier接口，供共享层配置转换下发）
     * @param    {vector<Osd::OverplayInfo_S>} &vecInfo：overplay信息
     * @return   {IpcRet_E} 0：成功 小于零：失败
     */
    IpcRet_E apply_overplay_info(const std::vector<Osd::OverplayInfo_S> &vecInfo) override;

    /***
     * @description : 获取cover配置信息
     * @author      : huangjunda
     * @param        {Osd::CoverConfig_S} &stInfo
     * @return       {IpcRet_E}
     * @note         : 转发共享层 COsdConfigure，保留旧接口兼容未迁移调用方
     */
    IpcRet_E get_cover_config(Osd::CoverConfig_S &stInfo);

    /**
     * @brief 获取当前平台实际支持的隐私遮盖区域数量
     *
     * 网页、ONVIF 和 TVSDK 应以该数量收敛配置，避免返回底层无法渲染的区域。
     */
    std::size_t get_cover_max_area_count() const override;

    /**
     * @brief   : 设置cover配置信息
     * @param    {Osd::CoverConfig_S} stInfo
     * @return   {IpcRet_E}
     * @note     : 转发共享层 COsdConfigure，保留旧接口兼容未迁移调用方
     */
    IpcRet_E set_cover_config(Osd::CoverConfig_S stInfo);

    /***
     * @description : 获取cover信息
     * @author      : huangjunda
     * @param        {vector<Osd::CoverInfo_S>} &vecInfo
     * @return       {IpcRet_E}
     */
    IpcRet_E get_cover_info(std::vector<Osd::CoverInfo_S> &vecInfo) override;

    /**
     * @brief   : 设置cover信息
     * @param    {vector<Osd::CoverInfo_S>} vecInfo：Cover信息组
     * @param    {bool} bIsWriteFile：是否将配置写入文件
     * @return   {IpcRet_E} 0：成功 小于零：失败
     */
    IpcRet_E set_cover_info(std::vector<Osd::CoverInfo_S> vecInfo, bool bIsWriteFile = true);

    /**
     * @brief   : 应用cover信息（IOsdConfigApplier接口，供共享层配置转换下发）
     * @param    {vector<Osd::CoverInfo_S>} &vecInfo：cover信息
     * @return   {IpcRet_E} 0：成功 小于零：失败
     */
    IpcRet_E apply_cover_info(const std::vector<Osd::CoverInfo_S> &vecInfo) override;

    /**
     * @brief   : OSD状态信息转换为Overplay渲染参数（IOsdConfigApplier平台差异钩子）
     * @param    {Osd::ElementType_E} enType：元素种类
     * @param    {std::string} &strText：元素文本
     * @param    {Osd::OsdAttribute_S} &stOsdAttr：OSD状态信息
     * @param    {Osd::Overplay_S} &stOverplay：Overplay渲染参数
     * @return   {IpcRet_E} 0：成功 小于零：失败
     * @note     : RK平台不透传宽高、不支持"黑白自动"反色，颜色串为RGB格式
     */
    IpcRet_E adapt_osd_attr(Osd::ElementType_E enType,
                            const std::string &strText,
                            Osd::OsdAttribute_S &stOsdAttr,
                            Osd::Overplay_S &stOverplay) override;

    /**
     * @brief   : Cover配置单区域映射为Cover渲染信息（IOsdConfigApplier平台差异钩子）
     * @param    {Osd::CoverAttribute_S} &stAttr：Cover区域配置
     * @param    {Osd::CoverInfo_S} &stInfo：Cover渲染信息
     * @return   {IpcRet_E} 0：成功 小于零：失败
     * @note     : RK平台颜色串为ARGB格式，坐标小于0的区域强制禁用
     */
    IpcRet_E cover_attr_to_info(const Osd::CoverAttribute_S &stAttr, Osd::CoverInfo_S &stInfo) override;

    /**
     * @brief   : 通知OSD共用信息更新，触发Overplay渲染刷新（IOsdConfigApplier接口）
     */
    void refresh_overplay() override;

    /***
     * @description : 获取osd共用信息
     * @author      : huangjunda
     * @param        {ShareInfo_S} &stuShareInfo
     * @return       {IpcRet_E}
     * @note         : 转发共享层 COsdConfigure，保留旧接口兼容未迁移调用方
     */
    IpcRet_E get_osd_share_info(Osd::ShareInfo_S &stuShareInfo);

    /***
     * @description : 设置osd共用信息
     * @author      : huangjunda
     * @param        {DeviceConfig_S} stDeviceConfig
     * @return       {IpcRet_E}
     * @note         : 转发共享层 COsdConfigure，保留旧接口兼容未迁移调用方
     */
    IpcRet_E set_osd_share_info(System::DeviceConfig_S stDeviceConfig);

    /**
     * @brief   : 发送 AI 检测结果，进行叠加展示
     * @param    {int} nWidth：检测结果框对应的分辨率宽
     * @param    {int} nHeight：检测结果框对应的分辨率高
     * @param    {vector<Common::RectInfo_S>} &vstRectInfo：矩形检测结果框
     * @return   {IpcRet_E} 0：成功 小于零：失败
     */
    IpcRet_E send_detection_result(const int nWidth, const int nHeight, const std::vector<Common::RectInfo_S> &vstRectInfo);

#if CAP_EXHIBITION_OSD_PANEL
    /**
     * @brief   : 发送展会面板结果，进行左上角汇总展示
     * @param    {const OsdPanel::PanelFrame_S &} stPanelFrame：结构化面板结果
     * @return   {IpcRet_E} 0：成功 小于零：失败
     */
    IpcRet_E send_panel_result(const OsdPanel::PanelFrame_S &stPanelFrame);

    /**
     * @brief   : 获取展会面板缓存
     * @param    {OsdPanel::PanelFrame_S &} stPanelFrame：输出面板结果
     * @param    {uint64_t &} unVersion：输出缓存版本
     * @param    {uint64_t &} unUpdateTimeMs：输出更新时间
     * @return   {IpcRet_E} 0：成功 小于零：失败
     */
    IpcRet_E get_panel_result(OsdPanel::PanelFrame_S &stPanelFrame, uint64_t &unVersion, uint64_t &unUpdateTimeMs);
#endif

    /***
     * @description : 重新加载osd
     * @author      : huangjunda
     * @param       : {int} nChn通道
     * @return       {void}
     */
    void reset_osd_status(int nChn);

    std::atomic<bool> m_bInit; /* 初始化标志 */
private:
    std::string m_strOverplayFile;                      /* overlay文件 */
    std::string m_strCoverFile;                         /* cover文件 */
    std::vector<Osd::OverplayInfo_S> m_vecOverplayInfo; /* overlay信息 */
    std::vector<Osd::CoverInfo_S> m_vecCoverInfo;       /* cover信息 */
    OS_MutexHndl m_stuMutex;                            /* 互斥锁 */
#if CAP_EXHIBITION_OSD_PANEL
    /* 展会面板缓存读写锁。 */
    std::mutex m_panelMutex;
    /* 当前缓存的最后一帧展会面板结果。 */
    OsdPanel::PanelFrame_S m_stPanelFrame;
    /* 展会面板缓存版本号。 */
    uint64_t m_unPanelVersion = 0;
    /* 展会面板最后一次刷新的稳态时间戳。 */
    uint64_t m_unPanelUpdateTimeMs = 0;
#endif
};

#endif // __OSD_MANAGE_H__
