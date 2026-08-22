/*
 * @FilePath     : sdk_new/sdk_server/src/business/BG6_ZHSJ/BU_SJGZ/SjgzDomain.cpp
 * @Author       : chenchl
 * @Date         : 2026-08-20
 * @LastEditors  : chenchl
 * @LastEditTime : 2026-08-20
 * @Description  : SJGZ 配置域实现
 *                 注册视频/报警/AI 命令码→处理函数映射。
 */
#ifndef BU_SJGZ_EXCLUDE

#include "SjgzDomain.h"
#include "IpcBusiness.h"

CSjgzDomain::CSjgzDomain()
{
    /* ==================================================================
     * Get 命令注册
     * ================================================================== */

    /* ===== IPC 专属（委托 CIpcBusiness） ===== */
    m_getTable[NET_GET_4G_INFO]        = [](INT32 ch, INT32 cmd, const std::string&, const std::string&) -> std::string {
        return CIpcBusiness::instance()->HandleGet4GInfo(ch, cmd);
    };
    m_getTable[NET_GET_HOTSPOT_CONN]   = [](INT32 ch, INT32 cmd, const std::string&, const std::string&) -> std::string {
        return CIpcBusiness::instance()->HandleGetHotspotConn(ch, cmd);
    };
    m_getTable[NET_GET_SD_CARD_STATUS] = [](INT32 ch, INT32 cmd, const std::string&, const std::string&) -> std::string {
        return CIpcBusiness::instance()->HandleGetSdCardStatus(ch, cmd);
    };

    /* ===== 视频流/OSD/图像参数 ===== */
    m_getTable[NET_GET_STREAMCFG]        = &CSjgzDomain::TemplatedGet<NET_VideoEncodeOption_S>;
    m_getTable[NET_GET_OSDCAPCFG]        = &CSjgzDomain::TemplatedGet<NET_VideoOsdCfg_S>;
    m_getTable[NET_GET_IMAGECFG]         = &CSjgzDomain::TemplatedGet<NET_ImageSetting_S>;
    m_getTable[NET_GET_PRIVACYMASKCFG]   = &CSjgzDomain::TemplatedGet<NET_PrivacyMaskCfg_S>;
    m_getTable[NET_GET_PREVIEW_INFO]     = &CSjgzDomain::TemplatedGet<NET_PreviewInfo_S>;


    /* ===== 图像参数（设备级） ===== */
    m_getTable[NET_GET_EXPOSURE_INFO]     = &CSjgzDomain::TemplatedGet<NET_ExposureInfo_S>;
    m_getTable[NET_GET_DAYNIGHT_INFO]     = &CSjgzDomain::TemplatedGet<NET_DayNightInfo_S>;
    m_getTable[NET_GET_BACKLIGHT_INFO]    = &CSjgzDomain::TemplatedGet<NET_BackLightInfo_S>;
    m_getTable[NET_GET_DENOISE_INFO]      = &CSjgzDomain::TemplatedGet<NET_DenoiseInfo_S>;
    m_getTable[NET_GET_WHITEBALANCE_INFO] = &CSjgzDomain::TemplatedGet<NET_WhiteBalanceInfo_S>;



    /* ==================================================================
     * Set 命令注册
     * ================================================================== */

    /* ===== IPC 专属（委托 CIpcBusiness） ===== */
    m_setTable[NET_SET_CONFIG_WIFI_STA]   = [](INT32 ch, INT32 cmd, const std::string& req, const std::string&) -> std::string {
        return CIpcBusiness::instance()->HandleSetWifiStaCfg(ch, cmd, req);
    };
    m_setTable[NET_CONNECT_WIFI_STA]      = [](INT32 ch, INT32 cmd, const std::string& req, const std::string&) -> std::string {
        return CIpcBusiness::instance()->HandleConnectWifiSta(ch, cmd, req);
    };
    m_setTable[NET_DISCONNECT_WIFI_STA]   = [](INT32 ch, INT32 cmd, const std::string& req, const std::string&) -> std::string {
        return CIpcBusiness::instance()->HandleDisconnectWifiSta(ch, cmd, req);
    };
    m_setTable[NET_SET_4G_INFO]           = [](INT32 ch, INT32 cmd, const std::string& req, const std::string&) -> std::string {
        return CIpcBusiness::instance()->HandleSet4GInfo(ch, cmd, req);
    };
    m_setTable[NET_SET_HOTSPOT_INFO]      = [](INT32 ch, INT32 cmd, const std::string& req, const std::string&) -> std::string {
        return CIpcBusiness::instance()->HandleSetHotspotInfo(ch, cmd, req);
    };

    /* ===== 视频流/OSD/图像参数 ===== */
    m_setTable[NET_SET_STREAMCFG]      = &CSjgzDomain::TemplatedSet<NET_VideoEncodeOption_S>;
    m_setTable[NET_SET_OSDCAPCFG]      = &CSjgzDomain::TemplatedSet<NET_VideoOsdCfg_S>;
    m_setTable[NET_SET_IMAGECFG]       = &CSjgzDomain::TemplatedSet<NET_ImageSetting_S>;
    m_setTable[NET_SET_PRIVACYMASKCFG]  = &CSjgzDomain::TemplatedSet<NET_PrivacyMaskCfg_S>;



    /* ===== 图像参数 ===== */
    m_setTable[NET_SET_EXPOSURE_INFO]     = &CSjgzDomain::TemplatedSet<NET_ExposureInfo_S>;
    m_setTable[NET_SET_DAYNIGHT_INFO]     = &CSjgzDomain::TemplatedSet<NET_DayNightInfo_S>;
    m_setTable[NET_SET_BACKLIGHT_INFO]    = &CSjgzDomain::TemplatedSet<NET_BackLightInfo_S>;
    m_setTable[NET_SET_DENOISE_INFO]      = &CSjgzDomain::TemplatedSet<NET_DenoiseInfo_S>;
    m_setTable[NET_SET_WHITEBALANCE_INFO] = &CSjgzDomain::TemplatedSet<NET_WhiteBalanceInfo_S>;

 
}

#endif /* BU_SJGZ_EXCLUDE */
