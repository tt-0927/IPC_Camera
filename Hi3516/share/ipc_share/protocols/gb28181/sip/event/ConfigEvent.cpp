/***
 * @FilePath     : ConfigEvent.cpp
 * @Author       : cyc
 * @Date         : 2025-07-02 09:33:30
 * @LastEditors  : cyc
 * @LastEditTime : 2025-07-03 14:21:11
 * @Description  : 配置事件
 */

#include "ConfigEvent.h"
#include "dlog.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
using namespace SIP;
bool ConfigEvent::Handle(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    /* 先响应200，说明已接收到请求 */
    SendResponse(e, SIP_OK);

    auto root = m_doc.first_child();
    /* 根据控制指令的细分进行判断 */
    if (!root.child("BasicParam").empty())
    {
        /* 设备配置-基本参数配置 */
        return HandleBasicParam(e);
    }else if(!root.child("VideoParamOpt").empty())
    {
        /* 设备配置-视频参数范围配置类型*/
        return HandleVideoParamOpt(e);
    }else if(!root.child("SVACEncodeConfig").empty())
    {
        /* 设备配置-SVAC编码配置*/
        return HandleSVACEncodeConfig(e);
    }else if(!root.child("SVACDncodeConfig").empty())
    {
        /* 设备配置-SVAC解码配置*/
        return HandleSVACDncodeConfig(e);
    }else if(!root.child("VideoParamAttribute").empty())
    {
        /* 设备配置-视频参数属性配置*/
        return HandleVideoParamAttribute(e);
    }else if(!root.child("VideoRecordPlan").empty())
    {
        /* 设备配置-录像计划配置*/
        return HandleVideoRecordPlan(e);
    }else if(!root.child("VideoAlarmRecord").empty())
    {
        /* 设备配置-报警录像配置*/
        return HandleVideoAlarmRecord(e);
    }else if(!root.child("PictureMask").empty())
    {
         /* 设备配置-视频画面遮挡配置*/
        return HandlePictureMask(e);
    }else if(!root.child("FrameMirror").empty())
    {
        /* 设备配置-视频画面翻转配置*/
        return HandleFrameMirror(e);
    }else if(!root.child("AlarmReport").empty())
    {
        /* 设备配置-报警上报开关配置*/
        return HandleAlarmReport(e);

    }else if(!root.child("OSDConfig").empty())
    {
        /* 设备配置-OSD配置*/
        return HandleOSDConfig(e);
    }else if(!root.child("SnapShotConfig").empty())
    {
        /* 设备配置-图像抓拍配置*/
        return HandleSnapShotConfig(e);
    }

    return true;
}

bool ConfigEvent::HandleBasicParam(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::BasicParamInfo_S tBasicParam;

    auto root = m_doc.first_child();
    /*序列号*/
    tBasicParam.nSN = root.child("SN").text().as_int();
    /* 设备Id */
    std::string strID;
    if(!root.child("DeviceID").empty())
    {
        strID = root.child("DeviceID").text().as_string();
    }
    tBasicParam.strID = strID;
    /*查询结果*/
    if(!root.child("Result").empty())
    {
        tBasicParam.strResult = root.child("Result").text().as_string();
    }
    /*通道标号*/
    tBasicParam.nIndex = -1;  //通道下标，下级默认-1
    /*true--设备配置设置，false--设备配置获取*/
    tBasicParam.bIsSet = true;

    auto node = root.child("BasicParam");

    if(!node.child("Name").empty())
    {
        tBasicParam.m_szName = node.child("Name").text().as_string();
    }
    tBasicParam.m_nExpiration = node.child("Expiration").text().as_int();
    tBasicParam.m_nHeartBeatInterval = node.child("HeartBeatInterval").text().as_int();
    tBasicParam.m_nHeartBeatCount = node.child("HeartBeatCount").text().as_int();

#if 0  //代码测试
    dlog_info("HandleBasicParam Message: DeviceID = %s\n",  tBasicParam.m_szName);
    dlog_info("HandleBasicParam Message: tBasicParam.m_nExpiration = %d\n",  tBasicParam.m_nExpiration);
    dlog_info("HandleBasicParam Message: tBasicParam.m_nHeartBeatInterval = %d\n",  tBasicParam.m_nHeartBeatInterval);
    dlog_info("HandleBasicParam Message: tBasicParam.m_nHeartBeatCount = %d\n",  tBasicParam.m_nHeartBeatCount);
#endif

    /*TODO 基本参数配置回调上抛*/
    { /* 配合上层实现业务功能 */
        SipCbResult_S stResult;
        auto fnCb = ::SipModule::instance()->GetBasicParamCb();
        if (fnCb)
        {
            fnCb(tBasicParam, stResult);
        }
    }

    return true;
}

bool ConfigEvent::HandleVideoParamOpt(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::VideoParamOptInfo_S tVideoParam;

    auto root = m_doc.first_child();
    /*序列号*/
    tVideoParam.nSN = root.child("SN").text().as_int();
    /* 设备Id */
    std::string strID;
    if(!root.child("DeviceID").empty())
    {
        strID = root.child("DeviceID").text().as_string();
    }
    tVideoParam.strID = strID;
    /*查询结果*/
    if(!root.child("Result").empty())
    {
        tVideoParam.strResult = root.child("Result").text().as_string();
    }
    /*通道标号*/
    tVideoParam.nIndex = -1;  //通道下标，下级默认-1
    /*true--设备配置设置，false--设备配置获取*/
    tVideoParam.bIsSet = false;

    auto node = root.child("VideoParamOpt");
    //下载倍数
    if(!node.child("DownloadSpeed").empty())
    {
        tVideoParam.strDownloadSpeed = node.child("DownloadSpeed").text().as_string();
    }
    //分辨率
    if(!node.child("Resolution").empty())
    {
        tVideoParam.strResolution = node.child("Resolution").text().as_string();
    }

#if 0  //代码测试
    dlog_info("HandleVideoParamOpt Message: DeviceID = %s\n",  tVideoParam.strID);
    dlog_info("HandleVideoParamOpt Message: tBasicParam.strResult = %s\n",  tVideoParam.strResult);
    dlog_info("HandleVideoParamOpt Message: tBasicParam.strDownloadSpeed = %d\n",  tVideoParam.strDownloadSpeed);
    dlog_info("HandleVideoParamOpt Message: tBasicParam.strResolution = %d\n",  tVideoParam.strResolution);
#endif

     /*TODO 基本参数配置回调上抛*/
    { /* 配合上层实现业务功能 */
        SipCbResult_S stResult;
        auto fnCb = ::SipModule::instance()->GetVideoParamOptCb();
        if (fnCb)
        {
            fnCb(tVideoParam, stResult);
        }
    }

     return true;
}

bool ConfigEvent::HandleSVACEncodeConfig(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::SVACEncodeInfo_S tSVACEncode;

    auto root = m_doc.first_child();
    /*序列号*/
    tSVACEncode.nSN = root.child("SN").text().as_int();
    /* 设备Id */
    std::string strID;
    if(!root.child("DeviceID").empty())
    {
        strID = root.child("DeviceID").text().as_string();
    }
    tSVACEncode.strID = strID;
    /*查询结果*/
    if(!root.child("Result").empty())
    {
        tSVACEncode.strResult = root.child("Result").text().as_string();
    }
    /*通道标号*/
    tSVACEncode.nIndex = -1;  //通道下标，下级默认-1
    /*true--设备配置设置，false--设备配置获取*/
    tSVACEncode.bIsSet = false;

    auto node = root.child("SVACEncodeConfig");

    //感兴趣区域参数
    {
        auto nodeROI = node.child("ROIParam");
        if (!nodeROI.empty())
        {
            //区域开关
            tSVACEncode.tROIParam.nROIFlag = nodeROI.child("ROIFlag").text().as_int();
            //区域数量
            tSVACEncode.tROIParam.nROINumber = nodeROI.child("ROINumber").text().as_int();
            auto children = nodeROI.children("Item");
            for (auto &&child : children)
            {
                GB28181::ROIParamItem Item;
                Item.nROISeq = child.child("ROISeq").text().as_int();
                Item.nTopLeft = child.child("TopLeft").text().as_int();
                Item.nBottomRight  = child.child("BottomRight").text().as_int();
                Item.nROIQP  = child.child("ROIQP").text().as_int();
                tSVACEncode.tROIParam.vecROIParamItem.push_back(Item);
            }
        }
    }

    //SVC参数
    {
        auto nodeSVC = node.child("SVCParam");
        if (!nodeSVC.empty())
        {
            //空域编码方式
            tSVACEncode.tSVCParam.nSVCSpaceDomainMode = nodeSVC.child("SVCSpaceDomainMode").text().as_int();
            //时域编码方式
            tSVACEncode.tSVCParam.nSVCTimeDomainMode = nodeSVC.child("SVCTimeDomainMode").text().as_int();
            //比例值
            if(nodeSVC.child("SSVCRatioValue"))
            {
                tSVACEncode.tSVCParam.strSSVCRatioValue = nodeSVC.child("SSVCRatioValue").text().as_string();
            }
            //空域编码能力
            tSVACEncode.tSVCParam.nSVCSpaceSupportMode = nodeSVC.child("SVCSpaceSupportMode").text().as_int();
             //时域编码能力
            tSVACEncode.tSVCParam.nSVCTimeSupportMode = nodeSVC.child("SVCTimeSupportMode").text().as_int();
            //比例能力
            if(nodeSVC.child("SSVCRatioSupportList"))
            {
                tSVACEncode.tSVCParam.strSSVCRatioSupportList = nodeSVC.child("SSVCRatioSupportList").text().as_string();
            }
        }
    }
    //监控专用信息参数
    {
        auto nodeSurveillance = node.child("SurveillanceParam");
        if (!nodeSurveillance.empty())
        {
            //时间信息开关
            tSVACEncode.tSurveillanceParam.nTimeFlag = nodeSurveillance.child("TimeFlag").text().as_int();
            tSVACEncode.tSurveillanceParam.nOSDFlag  = nodeSurveillance.child("OSDFlag").text().as_int();
            tSVACEncode.tSurveillanceParam.nAIFlag   = nodeSurveillance.child("AIFlag").text().as_int();
            tSVACEncode.tSurveillanceParam.nGISFlag   = nodeSurveillance.child("GISFlag").text().as_int();
        }
    }
    //音频参数
    {
        auto nodeAudio = node.child("AudioParam");
        if (!nodeAudio.empty())
        {
            //时间信息开关
            tSVACEncode.tAudioParam.nAudioRecognitionFlag = nodeAudio.child("AudioRecognitionFlag").text().as_int();
        }
    }

     /*TODO 基本参数配置回调上抛*/
    { /* 配合上层实现业务功能 */
        SipCbResult_S stResult;
        auto fnCb = ::SipModule::instance()->GetSVACEncodeCb();
        if (fnCb)
        {
            fnCb(tSVACEncode, stResult);
        }
    }

    return true;
}

bool ConfigEvent::HandleSVACDncodeConfig(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::SVACDecodeInfo_S tSVACDecode;

    auto root = m_doc.first_child();
    /*序列号*/
    tSVACDecode.nSN = root.child("SN").text().as_int();
    /* 设备Id */
    std::string strID;
    if(!root.child("DeviceID").empty())
    {
        strID = root.child("DeviceID").text().as_string();
    }
    tSVACDecode.strID = strID;
    /*查询结果*/
    if(!root.child("Result").empty())
    {
        tSVACDecode.strResult = root.child("Result").text().as_string();
    }
    /*通道标号*/
    tSVACDecode.nIndex = -1;  //通道下标，下级默认-1
    /*true--设备配置设置，false--设备配置获取*/
    tSVACDecode.bIsSet = false;

    auto node = root.child("SVACDecodeConfig");

    //SVC参数
    {
        auto nodeSVC = node.child("SVCParam");
        if (!nodeSVC.empty())
        {
            //码流显示模式
            tSVACDecode.tSVCParam.nSVCSTMMode = nodeSVC.child("SVCSTMMode").text().as_int();
            //空域编码能力
            tSVACDecode.tSVCParam.nSVCSpaceSupportMode = nodeSVC.child("SVCSpaceSupportMode").text().as_int();
             //时域编码能力
            tSVACDecode.tSVCParam.nSVCTimeSupportMode = nodeSVC.child("SVCTimeSupportMode").text().as_int();
        }
    }
    //监控专用信息参数
    {
        auto nodeSurveillance = node.child("SurveillanceParam");
        if (!nodeSurveillance.empty())
        {
            //时间信息开关
            tSVACDecode.tSurveillanceParam.nTimeShowFlag  = nodeSurveillance.child("TimeShowFlag").text().as_int();
            tSVACDecode.tSurveillanceParam.nOSDShowFlag   = nodeSurveillance.child("OSDShowFlag").text().as_int();
            tSVACDecode.tSurveillanceParam.nAIShowFlag    = nodeSurveillance.child("AIShowFlag").text().as_int();
            tSVACDecode.tSurveillanceParam.nGISShowFlag   = nodeSurveillance.child("GISShowFlag").text().as_int();
        }
    }

    /*TODO 基本参数配置回调上抛*/
    { /* 配合上层实现业务功能 */
        SipCbResult_S stResult;
        auto fnCb = ::SipModule::instance()->GetSVACDncodeCb();
        if (fnCb)
        {
            fnCb(tSVACDecode, stResult);
        }
    }

    return true;
}

bool ConfigEvent::HandleVideoParamAttribute(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::VideoParamAttributeInfo_S tAttribute;

    auto root = m_doc.first_child();
    /*序列号*/
    tAttribute.nSN = root.child("SN").text().as_int();
    /* 设备Id */
    std::string strID;
    if(!root.child("DeviceID").empty())
    {
        strID = root.child("DeviceID").text().as_string();
    }
    tAttribute.strID = strID;
    /*查询结果*/
    if(!root.child("Result").empty())
    {
        tAttribute.strResult = root.child("Result").text().as_string();
    }
    /*通道标号*/
    tAttribute.nIndex = -1;  //通道下标，下级默认-1
    /*true--设备配置设置，false--设备配置获取*/
    tAttribute.bIsSet = false;

    auto node = root.child("VideoParamAttribute");

    auto children = node.children("Item");
    for (auto &&child : children)
    {
        GB28181::VideoParamAttributeItem Item;
        //码流编号
        Item.nStreamNumber = child.child("StreamNumber").text().as_int();
        //视频编码格式
        if(!child.child("VideoFormat").empty())
        {
            Item.enVideoFormat = (GB28181::GbVideoType_E)child.child("VideoFormat").text().as_int();
        }
        //分辨率
        if(!child.child("Resolution").empty())
        {
            Item.enResolution  = (GB28181::GbResolutionType_E)child.child("Resolution").text().as_int();
        }
        //帧率
        if(!child.child("FrameRate").empty())
        {
            Item.strFrameRate  = child.child("FrameRate").text().as_string();
        }
        //码率类型
        if(!child.child("BitRateType").empty())
        {
            Item.enBitRateType = (GB28181::GbBitRateType_E)child.child("BitRateType").text().as_int();
        }
        //视频码率
        if(!child.child("VideoBitRate").empty())
        {
            Item.strVideoBitRate = child.child("VideoBitRate").text().as_string();
        }
        tAttribute.vecVideoParAttrItem.push_back(Item);
    }

     /*TODO 基本参数配置回调上抛*/
    { /* 配合上层实现业务功能 */
        SipCbResult_S stResult;
        auto fnCb = ::SipModule::instance()->GetVideoAttributeCb();
        if (fnCb)
        {
            fnCb(tAttribute, stResult);
        }
    }

     return true;
}

bool ConfigEvent::HandleVideoRecordPlan(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::VideoRecordPlanInfo_S tRecordPlan;

    auto root = m_doc.first_child();
    /*序列号*/
    tRecordPlan.nSN = root.child("SN").text().as_int();
    /* 设备Id */
    std::string strID;
    if(!root.child("DeviceID").empty())
    {
        strID = root.child("DeviceID").text().as_string();
    }
    tRecordPlan.strID = strID;
    /*查询结果*/
    if(!root.child("Result").empty())
    {
        tRecordPlan.strResult = root.child("Result").text().as_string();
    }
    /*通道标号*/
    tRecordPlan.nIndex = -1;  //通道下标，下级默认-1
    /*true--设备配置设置，false--设备配置获取*/
    tRecordPlan.bIsSet = false;

    auto node = root.child("VideoRecordPlan");

    /*录像启用,0-否，1-是*/
    tRecordPlan.nRecordEnable = node.child("RecordEnable").text().as_int();
     /*录像计划总天数*/
    tRecordPlan.nRecordScheduleSumNum = node.child("RecordScheduleSumNum").text().as_int();
    /*码流类型*/
    tRecordPlan.nStreamNumber = node.child("StreamNumber").text().as_int();

    auto nodeSchedulechil = node.children("RecordSchedule");
    for (auto &&childS : nodeSchedulechil)
    {
        //每一天的计划
        GB28181::RecordScheduleItem ScheduleItem;

        //周几
        ScheduleItem.nWeekDayNum = childS.child("WeekDayNum").text().as_int();
        //每天录像计划时间段总数
        ScheduleItem.nTimeSegmentSumNum = childS.child("TimeSegmentSumNum").text().as_int();
        auto childTime = childS.children("TimeSegment");
        for (auto &&childT : childTime)
        {
            GB28181::TimeSegmentItem  SegmentItem;
            /*开始时间时*/
            SegmentItem.nStartHour = childT.child("StartHour").text().as_int();
            /*开始时间分*/
            SegmentItem.nStartMin = childT.child("StartMin").text().as_int();
            /*开始时间秒*/
            SegmentItem.nStartSec = childT.child("StartSec").text().as_int();
            /*结束时间时*/
            SegmentItem.nStopHour = childT.child("StopHour").text().as_int();
             /*结束时间分*/
            SegmentItem.nStopMin = childT.child("nStopMin").text().as_int();
            /*结束时间秒*/
            SegmentItem.nStopSec = childT.child("StopSec").text().as_int();
            ScheduleItem.vecTimeSegment.push_back(SegmentItem);
        }
        tRecordPlan.vecRecordSchedule.insert(std::pair<int,  GB28181::RecordScheduleItem>(ScheduleItem.nWeekDayNum, ScheduleItem));
    }

    /*TODO 基本参数配置回调上抛*/
    { /* 配合上层实现业务功能 */
        SipCbResult_S stResult;
        auto fnCb = ::SipModule::instance()->GetRecordPlanCb();
        if (fnCb)
        {
            fnCb(tRecordPlan, stResult);
        }
    }

    return true;
}

bool ConfigEvent::HandleVideoAlarmRecord(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::VideoAlarmRecordInfo_S tAlarmRecord;

    auto root = m_doc.first_child();
    /*序列号*/
    tAlarmRecord.nSN = root.child("SN").text().as_int();
    /* 设备Id */
    std::string strID;
    if(!root.child("DeviceID").empty())
    {
        strID = root.child("DeviceID").text().as_string();
    }
    tAlarmRecord.strID = strID;
    /*查询结果*/
    if(!root.child("Result").empty())
    {
        tAlarmRecord.strResult = root.child("Result").text().as_string();
    }
    /*通道标号*/
    tAlarmRecord.nIndex = -1;  //通道下标，下级默认-1
    /*true--设备配置设置，false--设备配置获取*/
    tAlarmRecord.bIsSet = false;

    auto node = root.child("VideoAlarmRecord");

    tAlarmRecord.nRecordEnable = node.child("RecordEnable").text().as_int();
    tAlarmRecord.nRecordTime = node.child("RecordTime").text().as_int();
    tAlarmRecord.nPreRecordTime = node.child("PreRecordTime").text().as_int();
    tAlarmRecord.nStreamNumber = node.child("StreamNumber").text().as_int();

    /*TODO 基本参数配置回调上抛*/
    { /* 配合上层实现业务功能 */
        SipCbResult_S stResult;
        auto fnCb = ::SipModule::instance()->GetAlarmRecordCb();
        if (fnCb)
        {
            fnCb(tAlarmRecord, stResult);
        }
    }

     return true;
}

bool ConfigEvent::HandlePictureMask(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::PictureMaskInfo_S tPictureMask;

    auto root = m_doc.first_child();
    /*序列号*/
    tPictureMask.nSN = root.child("SN").text().as_int();
    /* 设备Id */
    std::string strID;
    if(!root.child("DeviceID").empty())
    {
        strID = root.child("DeviceID").text().as_string();
    }
    tPictureMask.strID = strID;
    /*查询结果*/
    if(!root.child("Result").empty())
    {
        tPictureMask.strResult = root.child("Result").text().as_string();
    }
    /*通道标号*/
    tPictureMask.nIndex = -1;  //通道下标，下级默认-1
    /*true--设备配置设置，false--设备配置获取*/
    tPictureMask.bIsSet = false;

    auto node = root.child("PictureMask");

    tPictureMask.On = node.child("On").text().as_int();
    tPictureMask.SumNum = node.child("SumNum").text().as_int();

    auto nodeRegionList = node.child("RegionList");

    //区域列表
    auto children = nodeRegionList.children("Item");
    for (auto &&child : children)
    {
        GB28181::RegionListItem Item;
        //区域编号
        Item.Seq = child.child("Seq").text().as_int();
        //区域左上角
        std::string piont;
        if(!child.child("Piont").empty())
        {
            piont = child.child("Piont").text().as_string();
        }
        std::istringstream iss(piont);
        std::string token;
        std::vector<int> ids;
        while (std::getline(iss, token, ',')) {
            int id = std::stoi(token);
            ids.push_back(id);
        }
        Item.nlx = ids[0];
        Item.nly = ids[1];
        Item.nrx = ids[2];
        Item.nry = ids[3];

        tPictureMask.vecRegionList.push_back(Item);
    }

     /*TODO 基本参数配置回调上抛*/
    { /* 配合上层实现业务功能 */
        SipCbResult_S stResult;
        auto fnCb = ::SipModule::instance()->GetPictureMaskCb();
        if (fnCb)
        {
            fnCb(tPictureMask, stResult);
        }
    }

     return true;
}

bool ConfigEvent::HandleFrameMirror(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::FrameMirrorInfo_S Mirror;

    auto root = m_doc.first_child();
    /*序列号*/
    Mirror.nSN = root.child("SN").text().as_int();
    /* 设备Id */
    std::string strID;
    if(!root.child("DeviceID").empty())
    {
        strID = root.child("DeviceID").text().as_string();
    }
    Mirror.strID = strID;
    /*查询结果*/
    if(!root.child("Result").empty())
    {
        Mirror.strResult = root.child("Result").text().as_string();
    }
    /*通道标号*/
    Mirror.nIndex = -1;  //通道下标，下级默认-1
    /*true--设备配置设置，false--设备配置获取*/
    Mirror.bIsSet = false;

    /*0-不启用镜像，1-水平镜像，2-上下镜像，3-中心镜像*/
    Mirror.nFrameMirror = root.child("FrameMirror").text().as_int();

    /*TODO 基本参数配置回调上抛*/
    { /* 配合上层实现业务功能 */
        SipCbResult_S stResult;
        auto fnCb = ::SipModule::instance()->GetFrameMirrorCb();
        if (fnCb)
        {
            fnCb(Mirror, stResult);
        }
    }

     return true;
}

bool ConfigEvent::HandleAlarmReport(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::AlarmReportInfo_S AlarmRepor;

    auto root = m_doc.first_child();
    /*序列号*/
    AlarmRepor.nSN = root.child("SN").text().as_int();
    /* 设备Id */
    std::string strID;
    if(!root.child("DeviceID").empty())
    {
        strID = root.child("DeviceID").text().as_string();
    }
    AlarmRepor.strID = strID;
    /*查询结果*/
    if(!root.child("Result").empty())
    {
        AlarmRepor.strResult = root.child("Result").text().as_string();
    }
    /*通道标号*/
    AlarmRepor.nIndex = -1;  //通道下标，下级默认-1
    /*true--设备配置设置，false--设备配置获取*/
    AlarmRepor.bIsSet = false;

    auto node = root.child("AlarmReport");

    /*移动侦测事件上报开关，0-关闭，1-打开*/
    AlarmRepor.nMotionDetection = node.child("MotionDetection").text().as_int();
    AlarmRepor.nFieldDetection = node.child("FieldDetection").text().as_int();

    /*TODO 基本参数配置回调上抛*/
    { /* 配合上层实现业务功能 */
        SipCbResult_S stResult;
        auto fnCb = ::SipModule::instance()->GetAlarmReportCb();
        if (fnCb)
        {
            fnCb(AlarmRepor, stResult);
        }
    }

    return true;
}

bool ConfigEvent::HandleOSDConfig(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::OSDConfig_S OSD;

    auto root = m_doc.first_child();
    /*序列号*/
    OSD.nSN = root.child("SN").text().as_int();
    /* 设备Id */
    std::string strID;
    if(!root.child("DeviceID").empty())
    {
        strID = root.child("DeviceID").text().as_string();
    }
    OSD.strID = strID;
    /*查询结果*/
    if(!root.child("Result").empty())
    {
        OSD.strResult = root.child("Result").text().as_string();
    }
    /*通道标号*/
    OSD.nIndex = -1;  //通道下标，下级默认-1
    /*true--设备配置设置，false--设备配置获取*/
    OSD.bIsSet = false;

    auto node = root.child("OSDConfig");

    OSD.m_nLength = node.child("Length").text().as_int();
    OSD.m_nWidth = node.child("Width").text().as_int();
    OSD.m_nTimeX = node.child("TimeX").text().as_int();
    OSD.m_nTimeY = node.child("TimeY").text().as_int();
    OSD.m_SumNum = node.child("SumNum").text().as_int();

    auto children = node.children("Item");
    for (auto &&child : children)
    {
        GB28181::OSDCItem  Item;
        if(!child.child("Text").empty())
        {
            Item.Text = child.child("Text").text().as_string();
        }
        Item.X = child.child("X").text().as_int();
        Item.Y = child.child("Y").text().as_int();
        OSD.m_vecItme.push_back(Item);
    }

    /*TODO 基本参数配置回调上抛*/
    { /* 配合上层实现业务功能 */
        SipCbResult_S stResult;
        auto fnCb = ::SipModule::instance()->GetOSDConfigCb();
        if (fnCb)
        {
            fnCb(OSD, stResult);
        }
    }

    return true;
}

bool ConfigEvent::HandleSnapShotConfig(const SipEvent::Ptr &e)
{
    ParseHeader(e);

    GB28181::SnapShotConfigInfo_S Snap;

    auto root = m_doc.first_child();
    /*序列号*/
    Snap.nSN = root.child("SN").text().as_int();
    /* 设备Id */
    std::string strID;
    if(!root.child("DeviceID").empty())
    {
        strID = root.child("DeviceID").text().as_string();
    }
    Snap.strID = strID;
    /*查询结果*/
    if(!root.child("Result").empty())
    {
        Snap.strResult = root.child("Result").text().as_string();
    }
    /*通道标号*/
    Snap.nIndex = -1;  //通道下标，下级默认-1
    /*true--设备配置设置，false--设备配置获取*/
    Snap.bIsSet = false;

    auto node = root.child("SnapShotConfig");

    Snap.nSnapNum = node.child("SnapNum").text().as_int();
    Snap.nInterval = node.child("Interval").text().as_int();
    if(!node.child("UploadURL").empty())
    {
        Snap.strUploadURL  = node.child("UploadURL").text().as_string();
    }
    if(!node.child("SessionID").empty())
    {
        Snap.strSessionID  = node.child("SessionID").text().as_string();
    }

    /*TODO 基本参数配置回调上抛*/
    { /* 配合上层实现业务功能 */
        SipCbResult_S stResult;
        auto fnCb = ::SipModule::instance()->GetSnapShotCb();
        if (fnCb)
        {
            fnCb(Snap, stResult);
        }
    }


    return true;
}
