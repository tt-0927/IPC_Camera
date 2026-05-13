/**
 * @FilePath     : gb28181.cpp
 * @Author       : zhouzirui
 * @Date         : 2025-05-28 19:21:51
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-10-13 16:51:14
 * @Description  : GB28181模块
 */

#include "gb28181.hpp"
#include "video_define.h"
#include "stream_video.h"
#include "system_define.h"
#include "system_manage.h"
#include "av_configure.h"
#include "task_publish.h"
#include "action_code.h"

CGB28181::CGB28181()
{
    m_strConfigPath = GB28181_CONFIG_FILE;
    m_vstIPInfo =  getLocalIPAddresses();
}

CGB28181::~CGB28181()
{
    
}

int CGB28181::init()
{
    Network::GB28181Client_S stGbClient;
    if (Convert::read_file(m_strConfigPath, stGbClient))
    {
        Convert::write_file(m_strConfigPath, stGbClient);
    }

    /* 初始化回调 */
    SIP::CbInfo_S stSipModuleInitInfo;
    initCallBack(stSipModuleInitInfo);

    set_GbClientConfig(stGbClient); 
    /*判断是否开启GB28181服务*/
    if (m_stGbClient.bEnableGB28181)
    {
        /* 开启日志的调试模式 */
        enable_module_log_print(1);
        
        // CbInfo_S stInitInfo;
        // stInitInfo.fnLog = nullptr;
        // stInitInfo.fnGetLocalInfo = [](SipLocalInfo_S &stInfo)
        // {
        //     System::DeviceInfo_S stDeviceInfo;
        //     SystemManage::instance()->get_device_info(stDeviceInfo);
        //     stInfo.strDevName = stDeviceInfo.deviceName;
        //     stInfo.strManufacturer = MANUFACTURER_NAME;
        //     stInfo.strModel = stDeviceInfo.strUnitTpye;
        //     stInfo.strFirmware = stDeviceInfo.hardwareVersion;
        // };
        SipModule::instance()->Init(stSipModuleInitInfo);

        SipClientInfo_S stClient;
        stClient.stLocal.nPort = m_stGbClient.nLocalSipPort;
        stClient.stLocal.strID = m_stGbClient.userId;
        stClient.stLocal.strPassword = m_stGbClient.password;
        stClient.stLocal.bTcp = m_stGbClient.bIsTcp;
        stClient.stLocal.strIP = m_vstIPInfo[0].ipAddress;

        stClient.stRemote.strID = m_stGbClient.serverID;
        stClient.stRemote.strIP = m_stGbClient.address;
        stClient.stRemote.nPort = m_stGbClient.nServerPort;
        stClient.stRemote.strRealm = m_stGbClient.realm;
        stClient.stRemote.strPassword = m_stGbClient.password;
        stClient.stRemote.nExpires = m_stGbClient.nValidityPeriod;
        stClient.stRemote.nHeartbeatInterval = m_stGbClient.nHeartbeatInterval;
        stClient.stRemote.nMaxHeartTimes = m_stGbClient.nMaxHeartbeatTimeout;
        SipModule::instance()->StartClient(stClient, m_stGbClient.bEnableGB35114);
        dlog_info("开启gb客户端");
        update_sipChannel();
    }
    else
    {
        SipModule::instance()->StopClient();
        m_bOnline = false;
        m_stGbClient.bStatus = m_bOnline;
        dlog_info("关闭gb客户端");
    }
    return OK;
}

int CGB28181::deinit()
{
    if (m_stGbClient.bEnableGB28181)
    {
        SipModule::instance()->StopClient();
        dlog_info("关闭gb客户端");
    }
    return OK;
}

void CGB28181::get_gbClientInfo(Network::GB28181Client_S &stGbClient)
{
    stGbClient = m_stGbClient;
    return;
}

int CGB28181::set_GbClientConfig(Network::GB28181Client_S stGbClient)
{
    m_stGbClient = stGbClient;
    m_stGbClient.bStatus = m_bOnline;
    return OK;
}

int CGB28181::update_sipChannel()
{
    std::vector<SipChannelInfo_S> vecInfo;
    /* TODO 更新通道消息 */
    SipChannelInfo_S stChnInfo;

    for(const auto &chnID:m_stGbClient.stEncode)
    {
        if(chnID.chnId.empty())
        {
            continue;
        }
        System::DeviceInfo_S stDeviceInfo;
        SystemManage::instance()->get_device_info(stDeviceInfo);
        stChnInfo.strChannelID = chnID.chnId;
        stChnInfo.strExternIP = m_vstIPInfo[0].ipAddress;
        stChnInfo.strStatus = "ON";
        stChnInfo.strName = stDeviceInfo.deviceName;
        stChnInfo.strModel = stDeviceInfo.strUnitTpye;
        stChnInfo.nIndex = 0;
        stChnInfo.strManufacturer = MANUFACTURER_NAME; /* (厂商名称，如海康则为HIKVISION) */
        vecInfo.push_back(stChnInfo);
    }
    
    SIP::SipModule::instance()->UpdateChnInfo(vecInfo);
    return OK;
}

std::vector<IPInfo> CGB28181::getLocalIPAddresses()
{
    std::vector<IPInfo> ipList;

    struct ifaddrs *ifaddr = nullptr;
    struct ifaddrs *ifa = nullptr;

    if (getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        return ipList;
    }

    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == nullptr)
            continue;

        int family = ifa->ifa_addr->sa_family;

        // 只取有地址且非回环的接口
        if ((family == AF_INET || family == AF_INET6) && !(ifa->ifa_flags & IFF_LOOPBACK))
        {
            char ip[INET6_ADDRSTRLEN] = {0};

            if (family == AF_INET)
            {
                // IPv4
                struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
                inet_ntop(AF_INET, &(sa->sin_addr), ip, INET_ADDRSTRLEN);
                ipList.push_back({ifa->ifa_name, ip, "IPv4"});
            }
            else if (family == AF_INET6)
            {
                // IPv6
                struct sockaddr_in6 *sa6 = (struct sockaddr_in6 *)ifa->ifa_addr;
                inet_ntop(AF_INET6, &(sa6->sin6_addr), ip, INET6_ADDRSTRLEN);
                ipList.push_back({ifa->ifa_name, ip, "IPv6"});
            }
        }
    }

    freeifaddrs(ifaddr);
    return ipList;
}

int CGB28181::initCallBack(SIP::CbInfo_S &stSipModuleInitInfo)
{
    {/* 设置GB模块的回调接口 */
            /* NOTE 必须设置设备信息回调函数，否则无法接收连接上来的设备信息 */
            stSipModuleInitInfo.fnDevUpdate = std::bind(
                &CGB28181::fn_gbDeviceInfo, this, std::placeholders::_1);
            stSipModuleInitInfo.fnGetLocalInfo = std::bind(
                &CGB28181::fn_gbGetLocalInfo, this, std::placeholders::_1);
            stSipModuleInitInfo.fnMediaUpdate = std::bind(
                &CGB28181::fn_gbMediaUpdate, this, std::placeholders::_1);
            stSipModuleInitInfo.fnMediaStatus = std::bind(
                &CGB28181::fn_gbMediaStatus, this, std::placeholders::_1);
            stSipModuleInitInfo.fnQueryRecordInfo = std::bind(
                &CGB28181::fn_gbQueryRecordInfo, this,
                std::placeholders::_1, std::placeholders::_2);
            stSipModuleInitInfo.fnGetRecFile = std::bind(
                &CGB28181::fn_gbGetRecFile, this,
                std::placeholders::_1, std::placeholders::_2);
            stSipModuleInitInfo.fnReadFileAction = std::bind(
                &CGB28181::fn_gbReadFileAction, this,
                std::placeholders::_1, std::placeholders::_2);
            stSipModuleInitInfo.fnPtzCmd = std::bind(
                &CGB28181::fn_gbPtzCmd, this,
                std::placeholders::_1, std::placeholders::_2);
            stSipModuleInitInfo.fnPreset = std::bind(
                &CGB28181::fn_gbPreset, this,
                std::placeholders::_1, std::placeholders::_2);
            stSipModuleInitInfo.fnPresetQuery = std::bind(
                &CGB28181::fn_gbPresetQuery, this,
                std::placeholders::_1, std::placeholders::_2);
            stSipModuleInitInfo.fnBasicParam = std::bind(
                &CGB28181::fn_gbBasicParamInfo, this,
                std::placeholders::_1, std::placeholders::_2);
            stSipModuleInitInfo.fnVideoParamOpt = std::bind(
                &CGB28181::fn_gbVideoParamOpt, this,
                std::placeholders::_1, std::placeholders::_2);
            stSipModuleInitInfo.fnSVACEncode = std::bind(
                &CGB28181::fn_gbSVACEncodeInfo, this,
                std::placeholders::_1, std::placeholders::_2);
            stSipModuleInitInfo.fnSVACDncode = std::bind(
                &CGB28181::fn_gbSVACDecodeInfo, this,
                std::placeholders::_1, std::placeholders::_2);
            stSipModuleInitInfo.fnVideoAttribute = std::bind(
                &CGB28181::fn_gbVideoParamAttributeInfo, this,
                std::placeholders::_1, std::placeholders::_2);
            stSipModuleInitInfo.fnRecordPlan = std::bind(
                &CGB28181::fn_gbVideoRecordPlanInfo, this,
                std::placeholders::_1, std::placeholders::_2);
            stSipModuleInitInfo.fnAlarmRecord = std::bind(
                &CGB28181::fn_gbVideoAlarmRecordInfo, this,
                std::placeholders::_1, std::placeholders::_2);
            stSipModuleInitInfo.fnPictureMask = std::bind(
                &CGB28181::fn_gbPictureMaskInfo, this,
                std::placeholders::_1, std::placeholders::_2);
            stSipModuleInitInfo.fnFrameMirror = std::bind(
                &CGB28181::fn_gbFrameMirrorInfo, this,
                std::placeholders::_1, std::placeholders::_2);
            stSipModuleInitInfo.fnAlarmReport = std::bind(
                &CGB28181::fn_gbAlarmReportInfo, this,
                std::placeholders::_1, std::placeholders::_2);
            stSipModuleInitInfo.fnOSDConfig = std::bind(
                &CGB28181::fn_gbOSDConfig, this,
                std::placeholders::_1, std::placeholders::_2);
            stSipModuleInitInfo.fnSnapShot = std::bind(
                &CGB28181::fn_gbSnapShotConfigInfo, this,
                std::placeholders::_1, std::placeholders::_2);
            stSipModuleInitInfo.fnCruiseTrackQuery = std::bind(
                &CGB28181::fn_gbCruiseTrackQueryInfo, this,
                std::placeholders::_1, std::placeholders::_2);
            stSipModuleInitInfo.fnCruiseTrackListQuery = std::bind(
                &CGB28181::fn_gbCruiseTrackListQueryInfo, this,
                std::placeholders::_1, std::placeholders::_2);
            stSipModuleInitInfo.fnBroadcast = std::bind(
                &CGB28181::fn_gbBroadcastInfo, this,
                std::placeholders::_1, std::placeholders::_2);
            stSipModuleInitInfo.fnSetTime = std::bind(
                &CGB28181::fn_gbSetTime, this,
                std::placeholders::_1, std::placeholders::_2);
            stSipModuleInitInfo.fnNVRTeleBoot = std::bind(
                &CGB28181::fn_gbTeleBootCb, this,
                std::placeholders::_1, std::placeholders::_2);
            stSipModuleInitInfo.fnIFrameCmd = std::bind(
                &CGB28181::fn_gbIFrameCmdCb, this,
                std::placeholders::_1, std::placeholders::_2);
            stSipModuleInitInfo.fnDragZoomInOut = std::bind(
                &CGB28181::fn_gbDragZoomInfo, this,
                std::placeholders::_1, std::placeholders::_2);
            stSipModuleInitInfo.fnPTZPreciseCtrl = std::bind(
                &CGB28181::fn_gbPTZPreciseCtrl, this,
                std::placeholders::_1, std::placeholders::_2);
             stSipModuleInitInfo.fnRecordCmd = std::bind(
                &CGB28181::fn_gbRecordCmd, this,
                std::placeholders::_1, std::placeholders::_2);
             stSipModuleInitInfo.fnControlResults = std::bind(
                &CGB28181::fn_gbControlResults, this,
                std::placeholders::_1, std::placeholders::_2);
             stSipModuleInitInfo.fnAlarmCmd = std::bind(
                &CGB28181::fn_gbAlarmCmd, this,
                std::placeholders::_1, std::placeholders::_2);
             stSipModuleInitInfo.fnHomePosition = std::bind(
                &CGB28181::fn_gbHomePositionInfo, this,
                std::placeholders::_1, std::placeholders::_2);
            stSipModuleInitInfo.fnDeviceUpgrade = std::bind(
                &CGB28181::fn_gbDeviceUpgradeInfo, this,
                std::placeholders::_1, std::placeholders::_2);
            stSipModuleInitInfo.fnTargetTrack = std::bind(
                &CGB28181::fn_gbTargetTrackInfo, this,
                std::placeholders::_1, std::placeholders::_2);
            stSipModuleInitInfo.fnPTZPosition = std::bind(
                &CGB28181::fn_gbPTZPositionInfo, this,
                std::placeholders::_1, std::placeholders::_2);
            stSipModuleInitInfo.fnOnlineStatus = std::bind(
                &CGB28181::fn_gbClineOnlineStatusInfo, this,
                std::placeholders::_1, std::placeholders::_2);
    }
    SIP::SipModule::instance()->Init(stSipModuleInitInfo);
    return OK;
}

void CGB28181::fn_gbDeviceInfo(SipDeviceInfo_S &stDevInfo)
{
    std::vector<Video_NS::VideoConfig_S> vstVideoConfig;

    CStreamVideo::instance()->getVideoConfig(vstVideoConfig);
    if(vstVideoConfig.empty())
    {
        return;
    }

    stDevInfo.nFps = vstVideoConfig[0].getFrameRateAsInt();

    if(vstVideoConfig[0].enVideoCodec == Video_NS::VideoCodec_E::H264)
    {
        stDevInfo.enVideo = SIP_VIDEO_H264;
    }
    else if(vstVideoConfig[0].enVideoCodec == Video_NS::VideoCodec_E::H265)
    {
        stDevInfo.enVideo = SIP_VIDEO_H265;
    }
    else
    {
        stDevInfo.enVideo = SIP_VIDEO_MPEG4;
    }
    
    stDevInfo.nWidth = vstVideoConfig[0].stVideoResolution.nWidth;
    stDevInfo.nHeight = vstVideoConfig[0].stVideoResolution.nHeight;
    
    return;

}

void CGB28181::fn_gbGetLocalInfo(SipLocalInfo_S &stInfo)
{
    System::DeviceInfo_S stDeviceInfo;
    SystemManage::instance()->get_device_info(stDeviceInfo);
    stInfo.strDevName = stDeviceInfo.deviceName;
    stInfo.strManufacturer = MANUFACTURER_NAME;
    stInfo.strModel = stDeviceInfo.strUnitTpye;
    stInfo.strFirmware = stDeviceInfo.hardwareVersion;

    return;
}

void CGB28181::fn_gbMediaUpdate(const SipMediaCbInfo_S &stInfo)
{
    Audio_NS::AoInfo_S stAoInfo;
    stAoInfo.nChannel = 0;
    stAoInfo.pData = (uint8_t *)stInfo.pData;
    stAoInfo.nLen = stInfo.nLen;
    stAoInfo.enAudioFormat = Audio_NS::AudioFormat_E::G711A;
    /* 接收到的广播数据发送到解码端 */
    CAVConfigure::instance()->setAoSpeakInfo(stAoInfo);
}

void CGB28181::fn_gbMediaStatus(const SipMediaStatus_S &stInfo)
{
    if(stInfo.bStart)
    {
        dlog_info("GB28181开始推流");
    }
    
}

void CGB28181::fn_gbQueryRecordInfo(
    const SipQueryRecCondition_S &stQuery,
    SipQueryRecResult_S &stResult)
{

}

void CGB28181::fn_gbGetRecFile(
    const SipGetRecFile_S &stInfo,
    SipCbResult_S &stResult)
{

}

void CGB28181::fn_gbReadFileAction(
    const SipReadFileAction_S &stInfo,
    SipCbResult_S &stResult)
{

}

void CGB28181::set_gbReadFileCodec(
    const std::string &strCallID,
    GB28181::GB28181CodecInfo_S &stCodec)
{
   
}
void CGB28181::fn_gbPtzCmd(
    const SipPtzInfoCb_S &stPtzInfo,
    SipCbResult_S &stResult)
{

}

void CGB28181::fn_gbPreset(
    const SipPresetInfoCb_S &stPresetInfo,
    SipCbResult_S &stResult)
{

}

void CGB28181::fn_gbPresetQuery(GB28181::PresetQueryInfo_S &stPresetQueryInfo,SipCbResult_S &stResult)
{

}

void CGB28181::fn_gbBasicParamInfo(GB28181::BasicParamInfo_S &stBasicParamInfo,SipCbResult_S &stResult)
{


}

void CGB28181::fn_gbVideoParamOpt(GB28181::VideoParamOptInfo_S &stVideoParamOptInfo,SipCbResult_S &stResult)
{

}

/* GB回调函数-SVAC编码配置 */
void CGB28181::fn_gbSVACEncodeInfo( GB28181::SVACEncodeInfo_S &stSVACEncodeInfo,SipCbResult_S &stResult)
{

}
/* GB回调函数-SVAC解码配置 */
void CGB28181::fn_gbSVACDecodeInfo(GB28181::SVACDecodeInfo_S &stSVACDecodeInfo,SipCbResult_S &stResult)
{

}
/* GB回调函数-视频参数 */
void CGB28181::fn_gbVideoParamAttributeInfo(GB28181::VideoParamAttributeInfo_S &stVideoParamAttributeInfo,SipCbResult_S &stResult)
{
    MLOG_INFO("视频参数设置");
    /* GB28181 协议参数与设备参数之间进行双向转换 */
    auto convert = [](GB28181::VideoParamAttributeItem& gb, Video_NS::VideoConfig_S& ipc, bool toGb)
    {
        if (toGb)
        {
            gb.nStreamNumber = ipc.nId;
            /* 编码格式转换 */
            if(ipc.enVideoCodec == Video_NS::VideoCodec_E::H264)
            {
                gb.enVideoFormat = GB28181::GbVideoType_E::GB_VIDEO_H264;
            }
            else if(ipc.enVideoCodec == Video_NS::VideoCodec_E::H265)
            {
                gb.enVideoFormat = GB28181::GbVideoType_E::GB_VIDEO_H265;
            }
            else if(ipc.enVideoCodec == Video_NS::VideoCodec_E::SVAC3)
            {
                gb.enVideoFormat = GB28181::GbVideoType_E::GB_VIDEO_SVAC;
            }
            else
            {
                gb.enVideoFormat = GB28181::GbVideoType_E::GB_VIDEO_H264;
            }
             /* 分辨率转换 */
            if(ipc.stVideoResolution.nHeight == PIXEL_HEIGHT_1080)
            {
                gb.enResolution = GB28181::GbResolutionType_E::GB_RESOLUTION_1080P;
            }
            else if(ipc.stVideoResolution.nHeight == PIXEL_HEIGHT_720)
            {
                gb.enResolution = GB28181::GbResolutionType_E::GB_RESOLUTION_720P;
            }

            /* 帧率 */
            gb.strFrameRate = std::to_string(ipc.getFrameRateAsInt());

            /* 码率类型 */
            if(ipc.enBitrateType == Video_NS::BitrateType_E::VBR)
            {
                gb.enBitRateType = GB28181::GbBitRateType_E::GB_RBITRATE_VBR;
            }
            else
            {
                gb.enBitRateType = GB28181::GbBitRateType_E::GB_RBITRATE_CBR;
            }
            
            /* 码率 */
            gb.strVideoBitRate = std::to_string(ipc.nAverageBitrate);
        }
        else
        {
            ipc.nId = gb.nStreamNumber;

            if(gb.enVideoFormat == GB28181::GbVideoType_E::GB_VIDEO_H264)
            {
                ipc.enVideoCodec = Video_NS::VideoCodec_E::H264;
            }
            else if(gb.enVideoFormat == GB28181::GbVideoType_E::GB_VIDEO_H265)
            {
                ipc.enVideoCodec = Video_NS::VideoCodec_E::H265;
            }
            else if(gb.enVideoFormat == GB28181::GbVideoType_E::GB_VIDEO_SVAC)
            {
                ipc.enVideoCodec = Video_NS::VideoCodec_E::SVAC3;
            }
            else
            {
                ipc.enVideoCodec = Video_NS::VideoCodec_E::H264;
            }

            if(gb.enResolution == GB28181::GbResolutionType_E::GB_RESOLUTION_1080P)
            {
                ipc.stVideoResolution.nWidth = PIXEL_WIDTH_1920;
                ipc.stVideoResolution.nHeight = PIXEL_HEIGHT_1080;
            }
            else if(gb.enResolution == GB28181::GbResolutionType_E::GB_RESOLUTION_720P)
            {
                ipc.stVideoResolution.nWidth = PIXEL_WIDTH_1280;
                ipc.stVideoResolution.nHeight = PIXEL_HEIGHT_720;
            }

            ipc.setFrameRate(std::stoi(gb.strFrameRate));
            if(gb.enBitRateType == GB28181::GbBitRateType_E::GB_RBITRATE_VBR)
            {
                ipc.enBitrateType = Video_NS::BitrateType_E::VBR;
            }
            else
            {
                ipc.enBitrateType = Video_NS::BitrateType_E::CBR;
            }

            ipc.nAverageBitrate  = std::stoi(gb.strVideoBitRate);
        }
    };

    int nRet = -1;
    std::vector<Video_NS::VideoConfig_S> vstVideoConfig;

    if(!stVideoParamAttributeInfo.bIsSet)
    {
        dlog_debug("[GB28181]获取通道[%d]视频参数配置",stVideoParamAttributeInfo.nIndex);

        nRet = CStreamVideo::instance()->getVideoConfig(vstVideoConfig);
        if(nRet != 0)
        {
            stVideoParamAttributeInfo.strResult = "ERROR";
            stResult.nResult = -1;
            return;
        }
        else
        {
            stVideoParamAttributeInfo.strResult = "OK";
            stResult.nResult = 0;
            GB28181::VideoParamAttributeItem stMainItem;
            GB28181::VideoParamAttributeItem stStubItem;
            convert(stMainItem, vstVideoConfig[STREAM_MEDIA_MAIN], true);
            convert(stStubItem, vstVideoConfig[STREAM_MEDIA_SUB], true);
            stVideoParamAttributeInfo.vecVideoParAttrItem.push_back(stMainItem);
            stVideoParamAttributeInfo.vecVideoParAttrItem.push_back(stStubItem);
        }
    }
    else
    {
        dlog_debug("[GB28181]设置通道[%d]视频参数配置",stVideoParamAttributeInfo.nIndex);
        if(stVideoParamAttributeInfo.vecVideoParAttrItem.size() < 2)
        {
            stVideoParamAttributeInfo.strResult = "ERROR";
            stResult.nResult = -1;
            return;
        }

        nRet = CStreamVideo::instance()->getVideoConfig(vstVideoConfig);
        if(nRet != 0)
        {
            stVideoParamAttributeInfo.strResult = "ERROR";
            stResult.nResult = -1;
            return;
        }

        convert(stVideoParamAttributeInfo.vecVideoParAttrItem[0], vstVideoConfig[STREAM_MEDIA_MAIN], false);
        convert(stVideoParamAttributeInfo.vecVideoParAttrItem[1], vstVideoConfig[STREAM_MEDIA_SUB], false);

        nRet = CStreamVideo::instance()->setVideoConfig(vstVideoConfig[STREAM_MEDIA_MAIN]);
        nRet = CStreamVideo::instance()->setVideoConfig(vstVideoConfig[STREAM_MEDIA_SUB]);
        if(nRet != 0)
        {
            stVideoParamAttributeInfo.strResult = "ERROR";
            stResult.nResult = -1;
            return;
        }
        else
        {
            stVideoParamAttributeInfo.strResult = "OK";
            stResult.nResult = 0;
        }
    }
}
/* GB回调函数-录像计划配置 */
void CGB28181::fn_gbVideoRecordPlanInfo(GB28181::VideoRecordPlanInfo_S &stVideoRecordPlanInfo,SipCbResult_S &stResult)
{

}
/* GB回调函数-报警录像配置 */
void CGB28181::fn_gbVideoAlarmRecordInfo(GB28181::VideoAlarmRecordInfo_S &stVideoAlarmRecordInfo,SipCbResult_S &stResult)
{

}
/* GB回调函数-视频画面遮挡配置 */
void CGB28181::fn_gbPictureMaskInfo(GB28181::PictureMaskInfo_S &stPictureMaskInfo,SipCbResult_S &stResult)
{

}
/* GB回调函数-视频画面翻转配置 */
void CGB28181::fn_gbFrameMirrorInfo(GB28181::FrameMirrorInfo_S &stFrameMirrorInfo,SipCbResult_S &stResult)
{

}
/* GB回调函数-报警上报开关配置 */
void CGB28181::fn_gbAlarmReportInfo(GB28181::AlarmReportInfo_S &stAlarmReportInfo,SipCbResult_S &stResult)
{

}
/* GB回调函数-OSD参数设置或获取 */
void CGB28181::fn_gbOSDConfig(GB28181::OSDConfig_S &stOSDConfig,SipCbResult_S &stResult)
{
    int nRet = -1;
    std::vector<Osd::OverplayInfo_S> vecOverplayInfo;

    System::DeviceConfig_S stDeviceConfig;
	SystemManage::instance()->get_device_config(stDeviceConfig);

    if(!stOSDConfig.bIsSet)
    {
        dlog_debug("[GB28181]获取通道[%d]OSD配置",stOSDConfig.nIndex);
	    nRet = COsdManage::instance()->get_overplay_info(vecOverplayInfo);
        if(nRet != ERR)
        {
            stOSDConfig.strResult = "ERROR";
            stResult.nResult = -1;
        }
        else
        {
            /* 窗口像素值 */
            switch(vecOverplayInfo[0].stuInfo.enRefSize)
            {
                case Osd::ReferenceSize_E::REFERENCE_SIZE_640_384:
                {
                    stOSDConfig.m_nWidth = PIXEL_WIDTH_640;
                    stOSDConfig.m_nLength = PIXEL_HEIGHT_384;
                    break;
                }
                case Osd::ReferenceSize_E::REFERENCE_SIZE_480P:
                {
                    stOSDConfig.m_nWidth = PIXEL_WIDTH_640;
                    stOSDConfig.m_nLength = PIXEL_HEIGHT_480;
                    break;
                }
                case Osd::ReferenceSize_E::REFERENCE_SIZE_640_640:
                {
                    stOSDConfig.m_nWidth = PIXEL_WIDTH_640;
                    stOSDConfig.m_nLength = PIXEL_HEIGHT_640;
                    break;
                }
                case Osd::ReferenceSize_E::REFERENCE_SIZE_576P:
                {
                    stOSDConfig.m_nWidth = PIXEL_WIDTH_1024;
                    stOSDConfig.m_nLength = PIXEL_HEIGHT_576;
                    break;
                }
                case Osd::ReferenceSize_E::REFERENCE_SIZE_720P:
                {
                    stOSDConfig.m_nWidth = PIXEL_WIDTH_1280;
                    stOSDConfig.m_nLength = PIXEL_HEIGHT_720;
                    break;
                }
                case Osd::ReferenceSize_E::REFERENCE_SIZE_1080P:
                {
                    stOSDConfig.m_nWidth = PIXEL_WIDTH_1920;
                    stOSDConfig.m_nLength = PIXEL_HEIGHT_1080;
                    break;
                }
                case Osd::ReferenceSize_E::REFERENCE_SIZE_2K:
                {
                    stOSDConfig.m_nWidth = PIXEL_WIDTH_2K;
                    stOSDConfig.m_nLength = PIXEL_HEIGHT_2K;
                    break;
                }
                case Osd::ReferenceSize_E::REFERENCE_SIZE_2_5K:
                {
                    stOSDConfig.m_nWidth = PIXEL_WIDTH_2_5K;
                    stOSDConfig.m_nLength = PIXEL_HEIGHT_2_5K;
                    break;
                }
                default:
                    break;
            }

            stOSDConfig.strResult = "OK";
            stResult.nResult = 0;

            /* 寻找时间的osd类型 */
            for (size_t i = 0; i < vecOverplayInfo.size(); i++)
            {
                /* osd类型为时间 */
                if(vecOverplayInfo[i].stuOverplay.enElementType == Osd::ELEMENT_TYPE_TIME)
                {
                    /* 时间坐标 */
                    stOSDConfig.m_nTimeX = vecOverplayInfo[i].stuOverplay.nHorMargin;
                    stOSDConfig.m_nTimeY = vecOverplayInfo[i].stuOverplay.nVerMargin;
                    stOSDConfig.m_nTimeEnable = vecOverplayInfo[i].stuInfo.bEnable;
                    /* 时间显示类型 */
                    if(stDeviceConfig.enDateFormat == System::DateFormat_E::YYYY_MM_DD && stDeviceConfig.enLanguage == System::Language_E::ENGLISH)
                    {
                        stOSDConfig.m_nTimeType = 0;
                    }
                    else
                    {
                        stOSDConfig.m_nTimeType = 1;   
                    }
                }
                else if(vecOverplayInfo[i].stuOverplay.enElementType == Osd::ELEMENT_TYPE_CUSTOMIZE)
                {
                    /* 显示文字开关 */
                    stOSDConfig.m_nTimeEnable = vecOverplayInfo[i].stuInfo.bEnable;
                    stOSDConfig.m_SumNum ++;
                    GB28181::OSDCItem stItem;
                    /* 文字内容 */
                    stItem.Text = vecOverplayInfo[i].stuOverplay.strCustomize;
                    /* 文字坐标 */
                    stItem.X = vecOverplayInfo[i].stuOverplay.nHorMargin;
                    stItem.Y = vecOverplayInfo[i].stuOverplay.nVerMargin;
                    stOSDConfig.m_vecItme.push_back(stItem);
                }
            }   

        }
    }
    else
    {
        dlog_debug("[GB28181]设置通道[%d]OSD配置",stOSDConfig.nIndex);

        stOSDConfig.m_nLength = stOSDConfig.m_nLength;
        stOSDConfig.m_nWidth = stOSDConfig.m_nWidth;

        if(stOSDConfig.m_nTimeEnable)
        {
            Osd::OverplayInfo_S stTimeInfo;
            stTimeInfo.stuOverplay.nHorMargin = stOSDConfig.m_nTimeX;
            stTimeInfo.stuOverplay.nVerMargin = stOSDConfig.m_nTimeY;
            stTimeInfo.stuOverplay.enElementType = Osd::ELEMENT_TYPE_TIME;
            stTimeInfo.stuInfo.bEnable = true;
            vecOverplayInfo.push_back(stTimeInfo);
        }

        stOSDConfig.m_nTimeEnable = stOSDConfig.m_nTextEnable;
        for (auto pItem : stOSDConfig.m_vecItme)
        {
            Osd::OverplayInfo_S stWordInfo;
            stWordInfo.stuInfo.bEnable = true;
            stWordInfo.stuOverplay.enElementType = Osd::ELEMENT_TYPE_CUSTOMIZE;
            stWordInfo.stuOverplay.strCustomize = pItem.Text;
            stWordInfo.stuOverplay.nHorMargin = pItem.X;
            stWordInfo.stuOverplay.nVerMargin = pItem.Y;
            vecOverplayInfo.push_back(stWordInfo);
        }

        nRet = COsdManage::instance()->set_overplay_info(vecOverplayInfo);
        if(nRet != 0)
        {
            stOSDConfig.strResult = "ERROR";
            stResult.nResult = -1;
        }
        else
        {
            stOSDConfig.strResult = "OK";
            stResult.nResult = 0;

        }
    }

}
/* GB回调函数-图像抓拍配置 */
void CGB28181::fn_gbSnapShotConfigInfo(GB28181::SnapShotConfigInfo_S &stSnapShotConfigInfo,SipCbResult_S &stResult)
{

}
/* GB回调函数-巡航轨迹查询 */
void CGB28181::fn_gbCruiseTrackQueryInfo(
    GB28181::CruiseTrackQueryInfo_S &stCruiseTrackQueryInfo,
    SipCbResult_S &stResult)
    {

    }
/* GB回调函数-巡航轨迹列表查询 */
void CGB28181::fn_gbCruiseTrackListQueryInfo(
    GB28181::CruiseTrackListQueryInfo_S &stCruiseTrackListQueryInfo,
    SipCbResult_S &stResult)
    {

    }
/* GB回调函数-语音广播 */
void CGB28181::fn_gbBroadcastInfo(
    GB28181::BroadcastInfo_S &stBroadcastInfo,
    SipCbResult_S &stResult)
    {
    }
/* GB回调函数-设备校时 */
void CGB28181::fn_gbSetTime(
    std::string &time,
    SipCbResult_S &stResult)
    {

    }

/* GB回调函数-远程启动 */
void CGB28181::fn_gbTeleBootCb(
    std::string &RemoteStart,
    SipCbResult_S &stResult)
    {
        if(RemoteStart == "Boot")
        {
            dlog_debug("GB回调-重启设备");
            stResult.nResult = 0;
            /* 重启 */
		    SystemManage::instance()->system_reboot([](int nRet) {});
        }
    }
/* GB回调函数-强制关键帧 */
void CGB28181::fn_gbIFrameCmdCb(
    GB28181::IFrame_S &stIFrame,
    SipCbResult_S &stResult)
    {
        if(stIFrame.IFrameCmd == "Send")
        {
            /* 默认主通道 */
            CStreamVideo::instance()->request_idr(0);
        }
        
    }
/* GB回调函数-拉框放大/缩小 */
void CGB28181::fn_gbDragZoomInfo(
    GB28181::DragZoomInfo_S &stDragZoomInfo,
    SipCbResult_S &stResult)
    {

    }
/* GB回调函数PTZ精准控制 */
void CGB28181::fn_gbPTZPreciseCtrl(
    GB28181::PTZPreciseCtrl_S &stPTZPreciseCtrl,
    SipCbResult_S &stResult)
    {

    }
/* GB回调函数-录像控制 */
void CGB28181::fn_gbRecordCmd(
    GB28181::RecordCmd_S &stRecordCmd,
    SipCbResult_S &stResult)
    {

    }
/* GB回调函数-设备控制应答 */
void CGB28181::fn_gbControlResults(
    GB28181::ControlResults_S &stControlResults,
    SipCbResult_S &stResult)
    {
        
    }
/*GB回调函数-报警复位控制命令*/
void CGB28181::fn_gbAlarmCmd(
    GB28181::AlarmCmd_S &stAlarmCmd,
    SipCbResult_S &stResult)
    {

    }
/*GB回调函数-看守位*/
void CGB28181::fn_gbHomePositionInfo(
    GB28181::HomePositionInfo_S &stHomePositionInfo,
    SipCbResult_S &stResult)
    {

    }
    /*GB回调函数-设备软件升级*/
void CGB28181::fn_gbDeviceUpgradeInfo(
    GB28181::DeviceUpgradeInfo_S &stDeviceUpgradeInfo,
    SipCbResult_S &stResult)
    {

    }
    /*GB回调函数-目标跟踪*/
void CGB28181::fn_gbTargetTrackInfo(
    GB28181::TargetTrackInfo_S &stTargetTrackInfo,
    SipCbResult_S &stResult)
    {
        
    }
    /*GB回调函数-PTZ精确状态查询*/
void CGB28181::fn_gbPTZPositionInfo(
    GB28181::PTZPositionInfo_S &stPTZPositionInfo,
    SipCbResult_S &stResult)
    {


    }
    /*GB回调函数-图像传输完成通知*/
void CGB28181::fn_gbUploadSnapShotFiniInfo(
    GB28181::UploadSnapShotFiniInfo_S &stUploadSnapShotFiniInfo,
    SipCbResult_S &stResult)
    {

    }

/*GB回调函数-客户端上线通知*/
void CGB28181::fn_gbClineOnlineStatusInfo(
    GB28181::GB28181ClientStatus_E &enClientStatus,
        SipCbResult_S &stResult)
{
    if(enClientStatus == GB28181::GB28181ClientStatus_E::ONLINE)
    {
        dlog_debug("gb客户端状态更新为在线");
        m_bOnline = true;
    }
    else
    {
        dlog_debug("gb客户端状态更新为离线");
        m_bOnline = false;
    }
    m_stGbClient.bStatus = m_bOnline;
    /* 主动通知网页上线状态 */
    TaskPublish::instance()->message(AC_GET_GB28181_INFO, Convert::to_string(m_stGbClient));
}