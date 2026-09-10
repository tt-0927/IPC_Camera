/**
 * @FilePath     : gb28181.hpp
 * @Author       : zhouzirui
 * @Date         : 2025-05-28 19:21:56
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-10-13 16:33:38
 * @Description  : GB28181模块
 */

#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <net/if.h>

#include "SipModule.h"
#include "Singleton.h"
#include "convert_interface.h"
#include "IpcRet.h"

/*IP信息*/
struct IPInfo
{
    std::string interfaceName;
    std::string ipAddress;
    std::string ipVersion; // "IPv4" 或 "IPv6"
};

using namespace SIP;

class CGB28181 : public CSingleton<CGB28181>
{
private:
    CGB28181();
public:
    ~CGB28181();
    friend class CSingleton<CGB28181>;

public:
    /**
     * @brief   : 初始化GB28181服务 
     * @return   int
     */
    int init();

    /**
     * @brief   : 去初始化GB28181服务  
     * @return   int
     */
    int deinit();

    void get_gbClientInfo(Network::GB28181Client_S &stGbClient);

    /**
     * @brief   : 设置gb28181客户端配置 
     * @param    {GB28181Client_S} stGbClient
     * @return   int
     */
    int set_GbClientConfig(Network::GB28181Client_S stGbClient);

     /*** 
     * @description : 更新通道sip信息
     * @author      : cyc
     * @return       {*}
     */    
    int update_sipChannel();
private:    
    /**
     * @brief   : 获取本地IP地址
     * @return   std::vector<IPInfo>
     */
    std::vector<IPInfo> getLocalIPAddresses();

    /*** 
     * @description : 初始化回调函数
     * @author      : cyc
     * @return       {*}
     */    
    int initCallBack(SIP::CbInfo_S &stSipModuleInitInfo);

    void fn_gbDeviceInfo(SipDeviceInfo_S &stDevInfo);
    void fn_gbGetLocalInfo(SipLocalInfo_S &stInfo);
    void fn_gbMediaUpdate(const SipMediaCbInfo_S &stInfo);
    void fn_gbMediaStatus(const SipMediaStatus_S &stInfo);
    void fn_gbQueryRecordInfo(
        const SipQueryRecCondition_S &stQuery,
        SipQueryRecResult_S &stResult);
    void fn_gbGetRecFile(
        const SipGetRecFile_S &stInfo,
        SipCbResult_S &stResult);
    void fn_gbReadFileAction(
        const SipReadFileAction_S &stInfo,
        SipCbResult_S &stResult);
    void set_gbReadFileCodec(
        const std::string &strCallID,
        GB28181::GB28181CodecInfo_S &stCodec);
    void fn_gbPtzCmd(
        const SipPtzInfoCb_S &stPtzInfo, 
        SipCbResult_S &stResult);
    void fn_gbPreset(
        const SipPresetInfoCb_S &stPresetInfo,
        SipCbResult_S &stResult);
     void fn_gbPresetQuery(
       GB28181::PresetQueryInfo_S &stPresetQueryInfo,
        SipCbResult_S &stResult);

    /* GB回调函数-基本参数配置 */
     void fn_gbBasicParamInfo(
       GB28181::BasicParamInfo_S &stBasicParamInfo,
        SipCbResult_S &stResult);
    /* GB回调函数-视频参数选项获取 */
    void fn_gbVideoParamOpt(
       GB28181::VideoParamOptInfo_S &stVideoParamOptInfo,
        SipCbResult_S &stResult);
    /* GB回调函数-SVAC编码配置 */
    void fn_gbSVACEncodeInfo(
       GB28181::SVACEncodeInfo_S &stSVACEncodeInfo,
        SipCbResult_S &stResult);
    /* GB回调函数-SVAC解码配置 */
    void fn_gbSVACDecodeInfo(
        GB28181::SVACDecodeInfo_S &stSVACDecodeInfo,
        SipCbResult_S &stResult);
    /* GB回调函数-视频参数获取 */
    void fn_gbVideoParamAttributeInfo(
       GB28181::VideoParamAttributeInfo_S &stVideoParamAttributeInfo,
        SipCbResult_S &stResult);
    /* GB回调函数-录像计划配置 */
    void fn_gbVideoRecordPlanInfo(
       GB28181::VideoRecordPlanInfo_S &stVideoRecordPlanInfo,
        SipCbResult_S &stResult);
    /* GB回调函数-报警录像配置 */
    void fn_gbVideoAlarmRecordInfo(
       GB28181::VideoAlarmRecordInfo_S &stVideoAlarmRecordInfo,
        SipCbResult_S &stResult);
     /* GB回调函数-视频画面遮挡配置 */
    void fn_gbPictureMaskInfo(
       GB28181::PictureMaskInfo_S &stPictureMaskInfo,
        SipCbResult_S &stResult);
    /* GB回调函数-视频画面翻转配置 */
    void fn_gbFrameMirrorInfo(
       GB28181::FrameMirrorInfo_S &stFrameMirrorInfo,
        SipCbResult_S &stResult);
    /* GB回调函数-报警上报开关配置 */
    void fn_gbAlarmReportInfo(
       GB28181::AlarmReportInfo_S &stAlarmReportInfo,
        SipCbResult_S &stResult);
    /* GB回调函数-OSD参数获取 */
    void fn_gbOSDConfig(
       GB28181::OSDConfig_S &stOSDConfig,
        SipCbResult_S &stResult);
    /* GB回调函数-图像抓拍配置 */
    void fn_gbSnapShotConfigInfo(
       GB28181::SnapShotConfigInfo_S &stSnapShotConfigInfo,
        SipCbResult_S &stResult);
    /* GB回调函数-巡航轨迹查询 */
    void fn_gbCruiseTrackQueryInfo(
       GB28181::CruiseTrackQueryInfo_S &stCruiseTrackQueryInfo,
        SipCbResult_S &stResult);
    /* GB回调函数-巡航轨迹列表查询 */
    void fn_gbCruiseTrackListQueryInfo(
       GB28181::CruiseTrackListQueryInfo_S &stCruiseTrackListQueryInfo,
        SipCbResult_S &stResult);
    /* GB回调函数-语音广播 */
    void fn_gbBroadcastInfo(
       GB28181::BroadcastInfo_S &stBroadcastInfo,
        SipCbResult_S &stResult);
    /* GB回调函数-设备校时 */
    void fn_gbSetTime(
       std::string &time,
        SipCbResult_S &stResult);
    /* GB回调函数-远程启动 */
    void fn_gbTeleBootCb(
       std::string &RemoteStart,
        SipCbResult_S &stResult);
    /* GB回调函数-强制关键帧 */
    void fn_gbIFrameCmdCb(
       GB28181::IFrame_S &stIFrame,
        SipCbResult_S &stResult);
    /* GB回调函数-拉框放大/缩小 */
    void fn_gbDragZoomInfo(
       GB28181::DragZoomInfo_S &stDragZoomInfo,
        SipCbResult_S &stResult);
    /* GB回调函数PTZ精准控制 */
    void fn_gbPTZPreciseCtrl(
       GB28181::PTZPreciseCtrl_S &stPTZPreciseCtrl,
        SipCbResult_S &stResult);
    /* GB回调函数-录像控制 */
    void fn_gbRecordCmd(
       GB28181::RecordCmd_S &stRecordCmd,
        SipCbResult_S &stResult);
    /* GB回调函数-设备控制应答 */
    void fn_gbControlResults(
       GB28181::ControlResults_S &stControlResults,
        SipCbResult_S &stResult);
    /*GB回调函数-报警复位控制命令*/
    void fn_gbAlarmCmd(
       GB28181::AlarmCmd_S &stAlarmCmd,
        SipCbResult_S &stResult);
    /*GB回调函数-看守位*/
    void fn_gbHomePositionInfo(
       GB28181::HomePositionInfo_S &stHomePositionInfo,
        SipCbResult_S &stResult);
     /*GB回调函数-设备软件升级*/
    void fn_gbDeviceUpgradeInfo(
       GB28181::DeviceUpgradeInfo_S &stDeviceUpgradeInfo,
        SipCbResult_S &stResult);
     /*GB回调函数-目标跟踪*/
    void fn_gbTargetTrackInfo(
       GB28181::TargetTrackInfo_S &stTargetTrackInfo,
        SipCbResult_S &stResult);
     /*GB回调函数-PTZ精确状态查询*/
    void fn_gbPTZPositionInfo(
       GB28181::PTZPositionInfo_S &stPTZPositionInfo,
        SipCbResult_S &stResult);
     /*GB回调函数-图像传输完成通知*/
    void fn_gbUploadSnapShotFiniInfo(
       GB28181::UploadSnapShotFiniInfo_S &stUploadSnapShotFiniInfo,
        SipCbResult_S &stResult);
    /*GB回调函数-上线状态通知*/
    void fn_gbClineOnlineStatusInfo(
        GB28181::GB28181ClientStatus_E &enClientStatus,
            SipCbResult_S &stResult);

private:
    /*配置文件路径*/
	std::string m_strConfigPath;
    /*gb28181客户端配置*/
    Network::GB28181Client_S m_stGbClient;
    /*IP信息*/
    std::vector<IPInfo> m_vstIPInfo;
    /* 在线注册状态 */
    bool m_bOnline = false;
};
