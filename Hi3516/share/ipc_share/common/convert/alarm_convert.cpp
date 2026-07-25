/**
 * @FilePath     : alarm_convert.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-07-17 17:25:12
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-04 10:52:47
 * @Description  : 报警配置数据的转换
 */

#include "alarm_convert.h"
#include "event_convert.h"
#include "common_convert.h"
#include "convert.h" /* 这个要放在UserDefineConvert的后面 */
#include "Json.h"
#include <iostream>

/* 联动方式相关 */
void Convert::deal(Json::Object *pRootJson, Alarm::LinkageType_E &enLinkageType, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "LinkageType", (int &)enLinkageType);
}

void Convert::deal(Json::Object *pRootJson, Alarm::LinkageList_S &stLinkageList, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Tradition", stLinkageList.tradition);
    convert.field(pRootJson, "AlarmLinkage", stLinkageList.alarmOutput);
    convert.field(pRootJson, "RecordChn", stLinkageList.recordChn);
    // convert.structure(pRootJson, stLinkageList.ptzLinkage);
    // convert.field(pRootJson, "AcousticLinkage", stLinkageList.acousticLinkage);
}

/**
 * @brief   : 普通事件
 */
/* 移动侦测相关 */
void Convert::deal(Json::Object *pRootJson, Alarm::MotionRegion_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "AreaNo", stInfo.nAreaNo);
    convert.structure(pRootJson, "Rect", stInfo.stRect);
    convert.field(pRootJson, "CloseSensitivity", stInfo.nCloseSensitivity);
    convert.field(pRootJson, "DaytimeSensitivity", stInfo.nDaytimeSensitivity);
    convert.field(pRootJson, "NightSensitivity", stInfo.nNightSensitivity);
}

void Convert::deal(Json::Object *pRootJson, std::vector<Alarm::MotionRegion_S> &vstInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "MotionRegion", vstInfo);
}

void Convert::deal(Json::Object *pRootJson, Alarm::MotionExpertMode_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "ExpertDayNightCtrl", stInfo.nExpertDayNightCtrl);
    convert.structure(pRootJson, "DayTime", stInfo.stDayTime);

    /* 移动侦测专家模式的区域参数 */
    if (!bOutStruct)
    {
        /* 判断移动侦测专家模式的区域参数是否刚初始化，没有就生成对应个数的区域 */
        if (stInfo.vstMotionRegion.size() == 0)
        {
            for (size_t i = 0; i < REGION_MAX; i++)
            {
                Alarm::MotionRegion_S stMotionRegion;
                stMotionRegion.nAreaNo = i + 1; // ID 从1开始
                stInfo.vstMotionRegion.emplace_back(stMotionRegion);
            }
        }
    }
    convert.structure(pRootJson, "MotionRegion", stInfo.vstMotionRegion);
}

void Convert::deal(Json::Object *pRootJson, Alarm::MotionNormalMode_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.field(pRootJson, "RegionType", stInfo.nRegionType);
    if (bOutStruct)
    {
        if(stInfo.nRegionType)
        {
            if (!std::holds_alternative<Alarm::MotionNormalMode_S::AreaGrid>(stInfo.varRegion))
            {
                stInfo.varRegion = Alarm::MotionNormalMode_S::AreaGrid();
            }
            auto &grid = std::get<Alarm::MotionNormalMode_S::AreaGrid>(stInfo.varRegion);
            Json::Object *pArray = Json::get(pRootJson, "Region");
            if (nullptr != pArray)
            {
                grid.clear();
                int nMax = Json::Array::size(pArray);
                for (int i = 0; i < nMax; i++)
                {
                    std::vector<unsigned int> pVec;
                    Json::Object *pArrayItem = Json::Array::get(pArray, i);
                    if (nullptr != pArrayItem)
                    {
                        int nItemMax = Json::Array::size(pArrayItem);
                        for (int j = 0; j < nItemMax; j++)
                        {
                            Json::Object *pItem = Json::Array::get(pArrayItem, j);
                            int nGet = 0;
                            Json::Value::get(pItem, nGet);
                            pVec.push_back(nGet);
                        }
                    }
                    grid.push_back(pVec);
                }
            }
        }
        else
        {
            if (!std::holds_alternative<Common::Rect_S>(stInfo.varRegion))
            {
                stInfo.varRegion = Common::Rect_S();
            }
            auto &rect = std::get<Common::Rect_S>(stInfo.varRegion);
            convert.structure(pRootJson, "Region", rect);
        }
    }
    else
    {
        if(stInfo.nRegionType)
        {
            if (!std::holds_alternative<Alarm::MotionNormalMode_S::AreaGrid>(stInfo.varRegion))
            {
                stInfo.varRegion = Alarm::MotionNormalMode_S::AreaGrid();
            }
            auto &grid = std::get<Alarm::MotionNormalMode_S::AreaGrid>(stInfo.varRegion);
            auto pArray = Json::Array::init();
            for (auto pVec : grid)
            {
                auto pArrayItem = Json::Array::init();
                if (!pArray)
                {
                    continue;
                }
                for (auto pItem : pVec)
                {
                    Json::Array::add(pArrayItem, (int &)pItem);
                }
                Json::Array::add(pArray, pArrayItem);
            }
            Json::add(pRootJson, "Region", pArray);
        }
        else
        {
            if (!std::holds_alternative<Common::Rect_S>(stInfo.varRegion))
            {
                stInfo.varRegion = Common::Rect_S();
            }
            auto &rect = std::get<Common::Rect_S>(stInfo.varRegion);
            convert.structure(pRootJson, "Region", rect);
        }
    }
}

void Convert::deal(Json::Object *pRootJson, Alarm::MotionDetection_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);

    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "DynamicAnalysisEnable", stInfo.bDynamicAnalysisEnable);
    convert.field(pRootJson, "Mode", (int &)stInfo.enMode);
    convert.structure(pRootJson, "MotionNormalMode", stInfo.stMotionNormalMode);
    convert.structure(pRootJson, "MotionExpertMode", stInfo.stMotionExpertMode);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
    if (bOutStruct)
    {
        int nAlarmNum = 1;
        while (1)
        {
            std::string strKey = "AlarmTime" + std::to_string(nAlarmNum);
            Json::Object *pItem = Json::get(pRootJson, strKey);
            if (nullptr == pItem)
            {
                break;
            }
            std::vector<Common::SchedTime_S> vecAlarmTime;
            int nMaxSize = Json::Array::size(pItem);
            for (int i = 0; i < nMaxSize; i++)
            {
                Common::SchedTime_S stAlarmTime;
                Json::Object *pArrayItem = Json::Array::get(pItem, i);
                if (nullptr == pArrayItem)
                {
                    continue;
                }

                convert.structure(pArrayItem, stAlarmTime);
                vecAlarmTime.push_back(stAlarmTime);
            }
            stInfo.aAlarmTime.push_back(vecAlarmTime);
            nAlarmNum++;
        }
    }
}


/* 遮挡报警相关 */
void Convert::deal(
    Json::Object *pRootJson,
    Alarm::HideAlarm_S &stInfo,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);

    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.structure(pRootJson, "Rect", stInfo.stRect);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}

/* 异常报警相关 */
void Convert::deal(Json::Object* pRootJson, Alarm::AbnormalDetection_S &stAbnormalDetection, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "AbnormalType", (int &) stAbnormalDetection.enAbnormalType);
    convert.structure(pRootJson, "LinkageMode", stAbnormalDetection.stLinkageList);
}

void Convert::deal(Json::Object* pRootJson, std::set<Alarm::AbnormalDetection_S> &abnormalDetection, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.structure(pRootJson, "AbnormalDetectionInfos", abnormalDetection);
}

/* 声音报警输出相关 */
void Convert::deal(Json::Object* pRootJson, std::vector<Alarm::CustomAudio_S> &stInfo, bool bOutStruct)
{
    if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "customAudio", stInfo);

}

void Convert::deal(Json::Object* pRootJson, Alarm::CustomAudio_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "enable", stInfo.bChoose);
    convert.field(pRootJson, "customeName", stInfo.strCustomeName);
    convert.field(pRootJson, "path", stInfo.strPath);

}

void Convert::deal(Json::Object* pRootJson, std::vector<Alarm::CustomOperation_S> &stInfo, bool bOutStruct)
{
    if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "CustomOperation", stInfo);
}

void Convert::deal(Json::Object* pRootJson, Alarm::CustomOperation_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "enable", stInfo.nEnable);
    convert.field(pRootJson, "Type", (int &)stInfo.enCustomType);
    convert.field(pRootJson, "customeName", stInfo.strName);
    convert.field(pRootJson, "fileName", stInfo.strFileName);
    convert.field(pRootJson, "path", stInfo.strPath);
}

void Convert::deal(Json::Object* pRootJson, Alarm::SoundOutputAlarm_S &stInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "SoundType", (int &)stInfo.enSoundType);
    convert.field(pRootJson, "AlertSound", (int &)stInfo.enAlertSound);
    convert.field(pRootJson, "Times", stInfo.nTimes);
    convert.structure(pRootJson, stInfo.aCustomAudio);
    convert.structure(pRootJson, stInfo.aAlarmTime);
}


/* 报警输入相关 */
void Convert::deal(Json::Object* pRootJson, Alarm::IoInputInfo_S &stIoInputInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "AlarmNumber", stIoInputInfo.nIoNumer);
	convert.field(pRootJson, "AlarmAddr", stIoInputInfo.ioAddr);
	convert.field(pRootJson, "AlarmName", stIoInputInfo.ioName);
	convert.field(pRootJson, "IsNormallyOpen", stIoInputInfo.bNormallyOpen);
	convert.field(pRootJson, "DealType", stIoInputInfo.nDealType);
    convert.structure(pRootJson, "LinkageMode", stIoInputInfo.stLinkageList);
	convert.structure(pRootJson, stIoInputInfo.aAlarmTime);
	convert.field(pRootJson, "CopyTo", stIoInputInfo.copyTo);
}
void Convert::deal(Json::Object* pRootJson, std::set<Alarm::IoInputInfo_S> &stIostIoInputInfoInputInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.structure(pRootJson, "AlarmInputInfos", stIostIoInputInfoInputInfo);
}


/* 报警输出相关 */
void Convert::deal(Json::Object* pRootJson, Alarm::IoOutputInfo_S &stIoOutputInfo, bool bOutStruct)
{
       if (!pRootJson)
       {
               return;
       }

       Convert::CConvert convert(bOutStruct);
       convert.field(pRootJson, "AlarmNumber", stIoOutputInfo.nIoNumer);
       convert.field(pRootJson, "AlarmAddr", stIoOutputInfo.ioAddr);
       convert.field(pRootJson, "AlarmName", stIoOutputInfo.ioName);
       convert.field(pRootJson, "DelayTime", stIoOutputInfo.nDelayTime);
       convert.field(pRootJson, "State", (int &)stIoOutputInfo.enState);
       convert.structure(pRootJson, stIoOutputInfo.aAlarmTime);
       convert.field(pRootJson, "CopyTo", stIoOutputInfo.copyTo);
}
void Convert::deal(Json::Object* pRootJson, std::set<Alarm::IoOutputInfo_S> &stIoOutputInfo, bool bOutStruct)
{
       if (!pRootJson)
       {
               return;
       }

       Convert::CConvert convert(bOutStruct);
       convert.structure(pRootJson, "AlarmOutputInfos", stIoOutputInfo);
}
/* 闪光报警相关 */
void Convert::deal(Json::Object *pRootJson, Alarm::FlashInfo_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
            return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "FlashTime", stInfo.nFlashTime);
    convert.field(pRootJson, "FalshFrequency",  (int &)stInfo.enFalshFrequency);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.field(pRootJson, "CopyTo", stInfo.copyTo);
}

/* PIR报警相关 */
void Convert::deal(Json::Object *pRootJson, Alarm::PirAlarmInfo_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
            return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "AlarmName", stInfo.AlarmName);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.field(pRootJson, "CopyTo", stInfo.copyTo);
}

/**
 * @brief   : 周界事件
 */

 /* 越界事件相关 */
 void Convert::deal(
    Json::Object *pRootJson,
    Alarm::BoundaryPlane_S &stInfo,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "StartPoint", stInfo.stStartPos);
    convert.structure(pRootJson, "EndPoint", stInfo.stEndPos);
    convert.field(pRootJson, "CrossDirection", (int &)stInfo.enCrossDirection);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.field(pRootJson, "DetectionTarget", stInfo.aDetectionTarget);

}
void Convert::deal(
    Json::Object *pRootJson,
    std::vector<Alarm::BoundaryPlane_S> &stInfo,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, stInfo);
}

void Convert::deal(
    Json::Object *pRootJson,
    Alarm::BoundaryDetection_S &stInfo,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.aRule);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}

/* 区域入侵相关 */
void Convert::deal(
    Json::Object *pRootJson,
    Alarm::Intrusion_S &stInfo,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "Region", stInfo.stRegion);
    convert.field(pRootJson, "Duration", stInfo.nTimeThreshold);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.field(pRootJson,"DetectionTarget",stInfo.aDetectionTarget);
}

void Convert::deal(
    Json::Object *pRootJson,
    std::vector<Alarm::Intrusion_S> &stInfo,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, stInfo);
}

void Convert::deal(
    Json::Object *pRootJson,
    Alarm::FieldDetection_S &stInfo,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);  
    convert.structure(pRootJson, "Rule",stInfo.aRule);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}

/* 进入区域相关 */
void Convert::deal(
    Json::Object *pRootJson,
    Alarm::EnterExitIntrusion_S &stInfo,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "Region", stInfo.stRegion);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.field(pRootJson, "DetectionTarget", stInfo.aDetectionTarget);
}

void Convert::deal(
    Json::Object *pRootJson,
    std::vector<Alarm::EnterExitIntrusion_S> &stInfo,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, stInfo);
}

void Convert::deal(
    Json::Object *pRootJson,
    Alarm::EntranceDetection_S &stInfo,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.aRule);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}

/* 离开区域相关 */
void Convert::deal(
    Json::Object *pRootJson,
    Alarm::ExitingDetection_S &stInfo,
    bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.aRule);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}

/**
 * @brief   : smart事件
 */

/* 音频异常侦测相关 */
void Convert::deal(Json::Object *pRootJson, Alarm::AudioAnomaly_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "AudioInputAnomaly", stInfo.bAudioInputAnomaly);
    convert.field(pRootJson, "UpEnable", stInfo.bUpEnable);
    convert.field(pRootJson, "UpSensitivity", stInfo.nUpSensitivity);
    convert.field(pRootJson, "UpThreshold", stInfo.nUpThreshold);
    convert.field(pRootJson, "DownEnable", stInfo.bDownEnable);
    convert.field(pRootJson, "DownSensitivity", stInfo.nDownSensitivity);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}

/* 场景变更侦测相关 */
void Convert::deal(Json::Object *pRootJson, Alarm::SceneChange_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}

/* 人脸侦测相关 */
void Convert::deal(Json::Object *pRootJson, Alarm::FaceDetection_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "DynamicAnalysisEnable", stInfo.bDynamicAnalysisEnable);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.structure(pRootJson, "Region", stInfo.stRegion);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}

/* 徘徊侦测相关 */
void Convert::deal(Json::Object *pRootJson, Alarm::LoiteringRule_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "Region", stInfo.stRegion);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.field(pRootJson, "TimeThreshold", stInfo.nTimeThreshold);
}

void Convert::deal(Json::Object *pRootJson, Alarm::LoiteringDetection_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.aRule);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}

/* 人员聚集侦测相关 */
void Convert::deal(Json::Object *pRootJson, Alarm::CrowdGatheringRule_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "Region", stInfo.stRegion);
    convert.field(pRootJson, "ObjectOccup", stInfo.nObjectOccup);
}

void Convert::deal(Json::Object *pRootJson, Alarm::CrowdGathering_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.aRule);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}

/* 停车侦测相关 */
void Convert::deal(Json::Object *pRootJson, Alarm::ParkingRule_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.field(pRootJson, "TimeThreshold", stInfo.nTimeThreshold);
    convert.structure(pRootJson, "Region", stInfo.stRegion);
}

void Convert::deal(Json::Object *pRootJson, Alarm::ParkingDetection_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.aRule);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}

/* 物品遗留侦测相关 */
void Convert::deal(Json::Object *pRootJson, Alarm::UnattendedObjectRule_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "Region", stInfo.stRegion);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.field(pRootJson, "TimeThreshold", stInfo.nTimeThreshold);
}

void Convert::deal(Json::Object *pRootJson, Alarm::UnattendedObject_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.aRule);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}

/* 物品拿取侦测相关 */
void Convert::deal(Json::Object *pRootJson, Alarm::ObjectRemovalRule_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "Region", stInfo.stRegion);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.field(pRootJson, "TimeThreshold", stInfo.nTimeThreshold);
}

void Convert::deal(Json::Object *pRootJson, Alarm::ObjectRemoval_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.aRule);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}

/* 宠物识别相关 */
void Convert::deal(Json::Object *pRootJson, Alarm::PetRecognition_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "DynamicAnalysisEnable", stInfo.bDynamicAnalysisEnable);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.structure(pRootJson, "Region", stInfo.stRegion);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}
void Convert::deal(Json::Object *pRootJson, Alarm::TargetLibInfos_S &TargetLibInfos, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "LibId", TargetLibInfos.LibId);
    convert.field(pRootJson, "Similarity", TargetLibInfos.Similarity);
    
}
/*人脸比对*/
void Convert::deal(Json::Object *pRootJson, Alarm::FaceCompare_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson,"TargetLibInfos", stInfo.TargetLibInfos);
    convert.structure(pRootJson, "LinkageSuccessMode", stInfo.stLinkageListSuccess);
    convert.structure(pRootJson, "LinkageFailMode", stInfo.stLinkageListFail);
}

/* 人脸抓拍相关 */
void Convert::deal(Json::Object *pRootJson, Alarm::FaceCaptureRule_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.structure(pRootJson, "Region", stInfo.stRegion);
    convert.structure(pRootJson, "ShieldedRegion", stInfo.vstShieldedRegion);
    convert.structure(pRootJson, "MinIpdRect", stInfo.stMinIpdRect);
    convert.field(pRootJson, "MinWidth", stInfo.nMinWidth);
    convert.field(pRootJson, "MinHeight", stInfo.nMinHeight);
    convert.field(pRootJson, "MaxWidth", stInfo.nMaxWidth);
    convert.field(pRootJson, "MaxHeight", stInfo.nMaxHeight);
    convert.field(pRootJson, "Interval", stInfo.nInterval);
}

void Convert::deal(Json::Object *pRootJson, Alarm::FaceCapture_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}

void Convert::deal(Json::Object *pRootJson, Alarm::OverlayInfo_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "DeviceID", stInfo.nDeviceID);
    convert.field(pRootJson, "MonitoryPointInfo", stInfo.strMonitoryPointInfo);
    convert.field(pRootJson, "OverlayDeviceID", stInfo.bOverlayDeviceID);
    convert.field(pRootJson, "OverlayCaptureTime", stInfo.bOverlayCaptureTime);
    convert.field(pRootJson, "OverlayMonitoryPointInfo", stInfo.bOverlayMonitoryPointInfo);
    convert.field(pRootJson, "EnFontColor", (int &)stInfo.enFontColor);
    convert.field(pRootJson, "StrFontColor", stInfo.strFontColor);
}


void Convert::deal(Json::Object *pRootJson, Alarm::FaceAlarmAttribute_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "IsMale", stInfo.bIsMale);
    convert.field(pRootJson, "AgeLabel", stInfo.nAgeLabel);
    convert.field(pRootJson, "IsGlasses", stInfo.bIsGlasses);
    convert.field(pRootJson, "IsBeard", stInfo.bIsBeard);
    convert.field(pRootJson, "IsMask", stInfo.bIsMask);
    convert.field(pRootJson, "EmotionLabel", stInfo.nEmotionLabel);
}

void Convert::deal(Json::Object *pRootJson, Alarm::FaceAlarmInfo_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "FaceAlarmAttribute", stInfo.stFaceAlarmAttribute);
    convert.structure(pRootJson, "FaceRegion", stInfo.stFaceRegion);
    convert.field(pRootJson, "FacePicture", stInfo.strFacePicture);
    convert.field(pRootJson, "CurrentPicture", stInfo.strCurrentPicture);
    convert.field(pRootJson, "TimeStamp", stInfo.strTimeStamp);  
    convert.field(pRootJson, "IsDownLoad", stInfo.bIsDownLoad); 
}

#ifdef SCENE_INTELLIGENT_ANALYSIS
/**
* @brief   : 场景智能分析
*/

/*场景智能分析控制*/
void Convert::deal(Json::Object *pRootJson, Alarm::LLMAISceneAnalysis_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "NewDialogue", stInfo.bNewDialogue);
    convert.field(pRootJson, "AnalysisStop", stInfo.bAnalysisStop);

}

/* 画面分析相关 */

/* 重复分析 */
void Convert::deal(Json::Object *pRootJson, Alarm::RepeatedAnalysisConfig_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Weekday", stInfo.aWeekdays);
    convert.structure(pRootJson, "RepeatedTime", stInfo.stExecuteTime);
}

/* 间隔分析 */
void Convert::deal(Json::Object *pRootJson, Alarm::IntervalAnalysisConfig_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "StartDate", stInfo.stStartDate);
    convert.structure(pRootJson, "EndDate", stInfo.stEndDate);
    convert.structure(pRootJson, "IntervalTime", stInfo.stIntervalTime);
}

/* 画面分析记录 */
void Convert::deal(Json::Object *pRootJson, Alarm::AnalysisRecords_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Id", stInfo.strId);
    convert.field(pRootJson, "CreateTime", stInfo.strCreateTime);
    convert.field(pRootJson, "InputText", stInfo.strInputText);
    convert.field(pRootJson, "OutputCreateTime", stInfo.strOutputCreateTime);
    convert.field(pRootJson, "OutputText", stInfo.strOutputText);
    convert.field(pRootJson, "SkipDelete", stInfo.bSkipDelete);
    convert.field(pRootJson, "InputImagePath", stInfo.strInputImagePath);
    convert.field(pRootJson, "VideoPath", stInfo.strVideoPath);
}

void Convert::deal(Json::Object *pRootJson, Alarm::AnalysisRecordIndexItem_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "indexKey", stInfo.indexKey);
    convert.field(pRootJson, "Cursor", stInfo.strCursor);
    convert.field(pRootJson, "TotalCount", stInfo.nTotalCount);
    convert.field(pRootJson, "HasMore", stInfo.bHasMore);
    convert.structure(pRootJson, "Records", stInfo.records);
}

void Convert::deal(Json::Object *pRootJson, Alarm::AnalysisAllRecordIndexItem_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Total_sessions", stInfo.total_sessions);
    convert.field(pRootJson, "Current_session_index", stInfo.current_session_index);
    convert.field(pRootJson, "Operateindex", stInfo.Operateindex);
    convert.field(pRootJson, "Operatesubindex", stInfo.Operatesubindex);
    convert.field(pRootJson, "AnalysisRecordOperate", (int &)stInfo.enAnalysisRecordOperate);
    convert.field(pRootJson, "SearchKeyword", stInfo.SearchKeyword);
    convert.field(pRootJson, "DelKeyID", stInfo.DelKeyID);
    convert.field(pRootJson, "PageSize", stInfo.nPageSize);
    convert.field(pRootJson, "PageIndex", stInfo.nPageIndex);
    convert.field(pRootJson, "Cursor", stInfo.strCursor);
    convert.field(pRootJson, "TotalCount", stInfo.nTotalCount);
    convert.field(pRootJson, "HasMore", stInfo.bHasMore);
    convert.structure(pRootJson, "AllRecords", stInfo.Allrecords);
}

void Convert::deal(Json::Object *pRootJson, Alarm::LLMImageAnalysis_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "AnalysisStop", stInfo.bAnalysisStop);
    convert.field(pRootJson, "ScreenshotEnable", stInfo.bScreenshotEnable);
    convert.field(pRootJson, "ScheduleEnable", stInfo.bScheduleEnable);
    convert.field(pRootJson, "AnalysisScheduleMode", (int &)stInfo.enAnalysisScheduleMode);
    convert.structure(pRootJson, "RepeatedAnalysis", stInfo.stRepeatedConfig);
    convert.structure(pRootJson, "IntervalAnalysis", stInfo.stIntervalConfig);
    convert.field(pRootJson, "AnalysisInputText", stInfo.strAnalysisInputText);
    convert.field(pRootJson, "AnalysisInputImagePath", stInfo.strAnalysisInputImagePath);

}

/* 文字预设任务相关 */
void Convert::deal(Json::Object *pRootJson, Alarm::TextPreset_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "OperateType", (int &)stInfo.enOperationType);
    convert.field(pRootJson, "TaskId", stInfo.strTaskId);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "TaskName", stInfo.strTaskName);
    convert.field(pRootJson, "TextPresetTaskStatus", (int &)stInfo.enTaskPresetDealStatus);
    convert.structure(pRootJson, "Rect", stInfo.stRect);
    convert.field(pRootJson, "UserObjectName", stInfo.strObjectName);
    convert.field(pRootJson, "UserConditionName", stInfo.strConditionName);
    convert.field(pRootJson, "DetectFrequency", (int &)stInfo.enDetectFrequency);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}

void Convert::deal(Json::Object *pRootJson, Alarm::TextPresetQueryFilter_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "TaskName", stInfo.strTaskNameFilter);
    convert.field(pRootJson, "UserObjectName", stInfo.strObjectNameFilter);
    convert.field(pRootJson, "UserConditionName", stInfo.strConditionNameFilter);
    convert.field(pRootJson, "TextPresetTaskStatus", (int &)stInfo.enTaskStatusFilter);
}

void Convert::deal(Json::Object *pRootJson, std::vector<Alarm::TextPreset_S> &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, stInfo);
}

void Convert::deal(Json::Object *pRootJson, Alarm::TextPresetTaskManager_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "ActiveId", stInfo.strCurrentActiveTaskId);
    convert.structure(pRootJson, "Tasks", stInfo.aTaskConfig);
}

/* 实时预警推送相关 */
void Convert::deal(Json::Object *pRootJson, Alarm::RealAlarmPushTime_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "Date", stInfo.stDate);
    convert.structure(pRootJson, "Time", stInfo.stTime);
}


void Convert::deal(Json::Object *pRootJson, Alarm::RealAlarmProcessRecord_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "ProcessUser", stInfo.strProcessUser);
    convert.field(pRootJson, "ProcessRemark", stInfo.strProcessRemark);
    convert.structure(pRootJson, "ProcessTime", stInfo.stProcessTime);
}

void Convert::deal(Json::Object *pRootJson, std::vector<Alarm::RealAlarmProcessRecord_S> &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, stInfo);
}

void Convert::deal(Json::Object *pRootJson, Alarm::RealAlarmPushRecord_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "OperateType", (int &)stInfo.enOperationType);
    convert.field(pRootJson, "TaskId", stInfo.strTaskId);
    convert.field(pRootJson, "TaskName", stInfo.strTaskName);
    convert.field(pRootJson, "UserObjectName", stInfo.strObjectName);
    convert.field(pRootJson, "UserConditionName", stInfo.strConditionName);
    convert.field(pRootJson, "DealStatus", (int &)stInfo.enDealStatus);
    convert.structure(pRootJson, "AlarmTime", stInfo.stAlarmTime);
    convert.field(pRootJson, "Description", stInfo.strDescription);
    convert.field(pRootJson, "ImagePath", stInfo.strImagePath);
    convert.field(pRootJson, "VideoPath", stInfo.strVideoPath);
    convert.structure(pRootJson, "ProcessRecord", stInfo.aProcessRecords);
}

void Convert::deal(Json::Object *pRootJson, std::vector<Alarm::RealAlarmPushRecord_S> &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "RealAlarmPushRecord", stInfo);
}

void Convert::deal(Json::Object *pRootJson, Alarm::RealAlarmPushManager_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "NotifyUpdate", stInfo.bNotifyUpdate);
    convert.field(pRootJson, "AutoLatestAlarm", stInfo.bAutoLaestAlarm);
    convert.field(pRootJson, "AutoPlay", stInfo.bAutoPlay);
    convert.field(pRootJson, "TotalCount", stInfo.nTotalCount);
    convert.field(pRootJson, "HasMore", stInfo.bHasMore);
    convert.structure(pRootJson, "RealAlarmPushRecord", stInfo.aPushRecords);
}

void Convert::deal(Json::Object *pRootJson, Alarm::RealAlarmPushQueryFilter_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "TaskName", stInfo.strTaskNameFilter);
    convert.field(pRootJson, "UserObjectName", stInfo.strObjectNameFilter);
    convert.field(pRootJson, "UserConditionName", stInfo.strConditionNameFilter);
    convert.field(pRootJson, "DealStatus", (int &)stInfo.enDealStatusFilter);
    convert.structure(pRootJson, "StartTime", stInfo.stStartTime);
    convert.structure(pRootJson, "EndTime", stInfo.stEndTime);
    convert.field(pRootJson, "PageSize", stInfo.nPageSize);
    convert.field(pRootJson, "PageIndex", stInfo.nPageIndex);
    convert.field(pRootJson, "Cursor", stInfo.strCursor);
}

void Convert::deal(Json::Object *pRootJson, Alarm::RealAlarmPushBatchRequest_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "OperateType", (int &)stInfo.enOperationType);
    convert.field(pRootJson, "TaskId", stInfo.aTaskIds);
    convert.field(pRootJson, "ProcessRemark", stInfo.strProcessRemark);
    convert.field(pRootJson, "AutoLatestAlarm", stInfo.bAutoLaestAlarm);
    convert.field(pRootJson, "AutoPlay", stInfo.bAutoPlay);
}
#endif

#ifdef SCENE_INTELLIGENCE
void Convert::deal(Json::Object *pRootJson, Alarm::FenceClimbingRule_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.structure(pRootJson, "Region", stInfo.stRegion);
}

void Convert::deal(Json::Object *pRootJson, Alarm::FenceClimbingDetection_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.aRule);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}

void Convert::deal(Json::Object *pRootJson, Alarm::LeavePostRule_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.field(pRootJson, "TimeThreshold", stInfo.nTimeThreshold);
    convert.structure(pRootJson, "Region", stInfo.stRegion);
}

void Convert::deal(Json::Object *pRootJson, Alarm::LeavePostDetection_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.aRule);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}

void Convert::deal(Json::Object *pRootJson, Alarm::PedestrianIntrusionRule_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.field(pRootJson, "TimeThreshold", stInfo.nTimeThreshold);
    convert.structure(pRootJson, "Region", stInfo.stRegion);
}

void Convert::deal(Json::Object *pRootJson, Alarm::PedestrianIntrusionDetection_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.aRule);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}

void Convert::deal(Json::Object *pRootJson, Alarm::SmokeFireRule_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
}

void Convert::deal(Json::Object *pRootJson, Alarm::SmokeFireDetection_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}

void Convert::deal(Json::Object *pRootJson, Alarm::OpenFlameRule_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
}

void Convert::deal(Json::Object *pRootJson, Alarm::OpenFlameDetection_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}

void Convert::deal(Json::Object *pRootJson, Alarm::RoadPondingRule_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
}

void Convert::deal(Json::Object *pRootJson, Alarm::RoadPondingDetection_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}

void Convert::deal(Json::Object *pRootJson, Alarm::ManholeCoverAbnormalRule_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
}
 
void Convert::deal(Json::Object *pRootJson, Alarm::ManholeCoverAbnormalDetection_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}

void Convert::deal(Json::Object *pRootJson, Alarm::SleepOnDutyRule_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
}
 
void Convert::deal(Json::Object *pRootJson, Alarm::SleepOnDutyDetection_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}

void Convert::deal(Json::Object *pRootJson, Alarm::TripRule_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
}
 
void Convert::deal(Json::Object *pRootJson, Alarm::TripDetection_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}

void Convert::deal(Json::Object *pRootJson, Alarm::PhoneUsageRule_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
}
 
void Convert::deal(Json::Object *pRootJson, Alarm::PhoneUsageDetection_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}

void Convert::deal(Json::Object *pRootJson, Alarm::PersonFallDownRule_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
}
 
void Convert::deal(Json::Object *pRootJson, Alarm::PersonFallDownDetection_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}


void Convert::deal(Json::Object *pRootJson, Alarm::HighAltitudeSeatbeltRule_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
}

void Convert::deal(Json::Object *pRootJson, Alarm::HighAltitudeSeatbeltDetection_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}

void Convert::deal(Json::Object *pRootJson, Alarm::LicensePlateCognitionRule_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
}

void Convert::deal(Json::Object *pRootJson, Alarm::LicensePlateCognitionDetection_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}

void Convert::deal(Json::Object *pRootJson, Alarm::BareSoilRule_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
}
 
void Convert::deal(Json::Object *pRootJson, Alarm::BareSoiletDection_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}

void Convert::deal(Json::Object *pRootJson, Alarm::SafetyHelmetRule_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
}

void Convert::deal(Json::Object *pRootJson, Alarm::SafetyHelmetDection_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}

void Convert::deal(Json::Object *pRootJson, Alarm::HoleProtectionBarRule_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
}

void Convert::deal(Json::Object *pRootJson, Alarm::HoleProtectionBarDection_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}

void Convert::deal(Json::Object *pRootJson, Alarm::ReflectiveClothingRule_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
}

void Convert::deal(Json::Object *pRootJson, Alarm::ReflectiveClothingDection_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}

void Convert::deal(Json::Object *pRootJson, Alarm::SmokingRule_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
}

void Convert::deal(Json::Object *pRootJson, Alarm::SmokingDection_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}

void Convert::deal(Json::Object *pRootJson, Alarm::ConstructionEncroachmentRoadRule_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
}

void Convert::deal(Json::Object *pRootJson, Alarm::ConstructionEncroachmentRoadDetection_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}

void Convert::deal(Json::Object *pRootJson, Alarm::ElectricScooterRule_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
}

void Convert::deal(Json::Object *pRootJson, Alarm::ElectricScooterDetection_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}

/* 逆行事件相关 */
 void Convert::deal(Json::Object *pRootJson, Alarm::DrivingAgainstTrafficRule_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "StartPoint", stInfo.stStartPos);
    convert.structure(pRootJson, "EndPoint", stInfo.stEndPos);
    convert.field(pRootJson, "CrossDirection", (int &)stInfo.enCrossDirection);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    // convert.field(pRootJson, "DetectionTarget", stInfo.aDetectionTarget);

}
void Convert::deal(Json::Object *pRootJson, std::vector<Alarm::DrivingAgainstTrafficRule_S> &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, stInfo);
}

void Convert::deal(Json::Object *pRootJson, Alarm::DrivingAgainstTrafficDetection_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.aRule);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}

/* 拥堵事件相关 */
void Convert::deal(Json::Object *pRootJson, Alarm::CongestionRule_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
}

void Convert::deal(Json::Object *pRootJson, Alarm::CongestionDetection_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}

/* 违规变道事件相关 */
 void Convert::deal(Json::Object *pRootJson, Alarm::IllegalLaneChangeRule_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "StartPoint", stInfo.stStartPos);
    convert.structure(pRootJson, "EndPoint", stInfo.stEndPos);
    // convert.field(pRootJson, "CrossDirection", (int &)stInfo.enCrossDirection);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    // convert.field(pRootJson, "DetectionTarget", stInfo.aDetectionTarget);

}
void Convert::deal(Json::Object *pRootJson, std::vector<Alarm::IllegalLaneChangeRule_S> &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, stInfo);
}

void Convert::deal(Json::Object *pRootJson, Alarm::IllegalLaneChangeDetection_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.aRule);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}

/* 应急车道占用侦测相关 */
void Convert::deal(Json::Object *pRootJson, Alarm::EmergencyLaneOccupancyRule_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.field(pRootJson, "TimeThreshold", stInfo.nTimeThreshold);
    convert.structure(pRootJson, "Region", stInfo.stRegion);
    convert.field(pRootJson, "DetectionTarget", stInfo.aDetectionTarget);
}

void Convert::deal(Json::Object *pRootJson, Alarm::EmergencyLaneOccupancyDetection_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.aRule);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}

/* 非机动车闯入侦测相关 */
void Convert::deal(Json::Object *pRootJson, Alarm::NonMotorVehicleIntrusionRule_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.field(pRootJson, "TimeThreshold", stInfo.nTimeThreshold);
    convert.structure(pRootJson, "Region", stInfo.stRegion);
}

void Convert::deal(Json::Object *pRootJson, Alarm::NonMotorVehicleIntrusionDetection_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.aRule);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}

void Convert::deal(Json::Object *pRootJson, Alarm::PersonAlarmAttribute_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "IsMale", stInfo.bIsMale);
    convert.field(pRootJson, "AgeLabel", stInfo.nAgeLabel);
    convert.field(pRootJson, "BottomColorLabel", (int &)stInfo.eBottomColorLabel);
    convert.field(pRootJson, "TopColorLabel", (int &)stInfo.eTopColorLabel);
    convert.field(pRootJson, "IsBag", stInfo.bBag);
}

void Convert::deal(Json::Object *pRootJson, Alarm::PersonAlarmInfo_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "PersonAlarmAttribute", stInfo.stPersonAlarmAttribute);
    convert.field(pRootJson, "PersonPicture", stInfo.strPersonPicture);
    convert.field(pRootJson, "CurrentPicture", stInfo.strCurrentPicture);
    convert.field(pRootJson, "TimeStamp", stInfo.strTimeStamp);  
    convert.field(pRootJson, "IsDownLoad", stInfo.bIsDownLoad); 
}

void Convert::deal(Json::Object *pRootJson, Alarm::MotorvehicleAlarmAttribute_S &stInfo, bool bOutStruct)
{ 
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "VehicleType", (int &)stInfo.eVehicleType);
    convert.field(pRootJson, "VehicleColor", (int &)stInfo.eVehicleColor);
    convert.field(pRootJson, "VehicleBrand", stInfo.strVehicleBrand);
}

void Convert::deal(Json::Object *pRootJson, Alarm::MotorvehicleAlarmInfo_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);

    convert.structure(pRootJson, "MotorvehicleAlarmAttribute", stInfo.stMotorvehicleAlarmAttribute);
    convert.field(pRootJson, "LicensePlateNumber", stInfo.strLicensePlateNumber);
    convert.field(pRootJson, "TargetPicture", stInfo.strTargetPicture);
    convert.field(pRootJson, "CurrentPicture", stInfo.strCurrentPicture);
    convert.field(pRootJson, "TimeStamp", stInfo.strTimeStamp);  
    convert.field(pRootJson, "IsDownLoad", stInfo.bIsDownLoad); 
}

void Convert::deal(Json::Object *pRootJson, Alarm::NonMotorvehicleAlarmAttribute_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "NonMotorizedVehicleType", (int &)stInfo.eNonMotorizedVehicleType);
    convert.field(pRootJson, "NonMotorizedVehicleColor", (int &)stInfo.eNonMotorizedVehicleColor);
}

void Convert::deal(Json::Object *pRootJson, Alarm::NonMotorvehicleAlarmInfo_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "NonMotorvehicleAlarmAttribute", stInfo.stNonMotorvehicleAlarmAttribute);
    convert.field(pRootJson, "TargetPicture", stInfo.strTargetPicture);
    convert.field(pRootJson, "CurrentPicture", stInfo.strCurrentPicture);
    convert.field(pRootJson, "TimeStamp", stInfo.strTimeStamp);  
    convert.field(pRootJson, "IsDownLoad", stInfo.bIsDownLoad); 
}

void Convert::deal(Json::Object *pRootJson, Alarm::AttributeDetectSwitch_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "MotorVehicleAttribute", stInfo.bMotorVehicleAttribute);
    convert.field(pRootJson, "NonMotorVehicleAttribute", stInfo.bNonMotorVehicleAttribute);
    convert.field(pRootJson, "FaceAttribute", stInfo.bFaceAttribute);
    convert.field(pRootJson, "PedestrianAttribute", stInfo.bPedestrianAttribute);
}

#endif

#if defined(SCENE_INTELLIGENCE) || CAP_AI_GARBAGE_DETECT
void Convert::deal(Json::Object *pRootJson, Alarm::GarbageExposureRule_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.structure(pRootJson, "Region", stInfo.stRegion);
}

void Convert::deal(Json::Object *pRootJson, Alarm::GarbageExposureDetection_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}

void Convert::deal(Json::Object *pRootJson, Alarm::GarbageOverflowRule_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.structure(pRootJson, "Region", stInfo.stRegion);
}

void Convert::deal(Json::Object *pRootJson, Alarm::GarbageOverflowDetection_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "Rule", stInfo.stRule);
    convert.structure(pRootJson, stInfo.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}
#endif

/* 人数统计相关 */
#if CAP_AI_PEOPLE_STATISTICS
void Convert::deal(Json::Object *pRootJson, Alarm::PeopleFlowRuleLine_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "StartPoint", stInfo.stStartPos);
    convert.structure(pRootJson, "EndPoint", stInfo.stEndPos);
    convert.field(pRootJson, "CrossDirection", (int &) stInfo.enDirection);
}

void Convert::deal(Json::Object *pRootJson, Alarm::PopulationAlarmRule_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "Threshold", stInfo.nThreshold);
    convert.structure(pRootJson, "LinkageMode", stInfo.stLinkageList);
}

void Convert::deal(Json::Object *pRootJson, Alarm::PopulationAlarmConfig_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "Normal", stInfo.stNormal);
    convert.structure(pRootJson, "Medium", stInfo.stMedium);
    convert.structure(pRootJson, "Severe", stInfo.stSevere);
}

void Convert::deal(Json::Object *pRootJson, Alarm::StatisticsResetConfig_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.structure(pRootJson, "ExecuteTime", stInfo.stExecuteTime);
}

void Convert::deal(Json::Object *pRootJson, Alarm::PeopleFlowStatistics_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.structure(pRootJson, "RuleLine", stInfo.stRuleLine);
    convert.structure(pRootJson, "Region", stInfo.stDetectRegion);
    convert.field(pRootJson, "ReportInterval", stInfo.nReportInterval);
    convert.field(pRootJson, "StatisticsType", (int &) stInfo.enStatisticsType);
    convert.structure(pRootJson, "TimedReset", stInfo.stTimedReset);
    convert.structure(pRootJson, "StayAlarm", stInfo.stStayAlarm);
    convert.structure(pRootJson, stInfo.aAlarmTime);
}

void Convert::deal(Json::Object *pRootJson, Alarm::PeopleDensityDetection_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Enable", stInfo.bEnable);
    convert.field(pRootJson, "Sensitivity", stInfo.nSensitivity);
    convert.structure(pRootJson, "Region", stInfo.stDetectRegion);
    convert.field(pRootJson, "ReportInterval", stInfo.nReportInterval);
    convert.structure(pRootJson, "DensityAlarm", stInfo.stDensityAlarm);
    convert.structure(pRootJson, stInfo.aAlarmTime);
}
#endif

void Convert::deal(Json::Object *pRootJson, std::vector<std::vector<Common::SchedTime_S>> &vecInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    if (bOutStruct)
    {
        int nAlarmNum = 1;
        while (1)
        {
            std::string strKey = "AlarmTime" + std::to_string(nAlarmNum);
            Json::Object *pItem = Json::get(pRootJson, strKey);
            if (nullptr == pItem)
            {
                break;
            }
            std::vector<Common::SchedTime_S> vecAlarmTime;
            int nMaxSize = Json::Array::size(pItem);
            for (int i = 0; i < nMaxSize; i++)
            {
                Common::SchedTime_S stAlarmTime;
                Json::Object *pArrayItem = Json::Array::get(pItem, i);
                if (nullptr == pArrayItem)
                {
                    continue;
                }

                convert.structure(pArrayItem, stAlarmTime);
                vecAlarmTime.push_back(stAlarmTime);
            }
            vecInfo.push_back(vecAlarmTime);
            nAlarmNum++;
        }
    }
    else
    {
        int nMaxSize = std::min((int) vecInfo.size(), WEEK_DAYS);
        for (int i = 0; i < nMaxSize; i++)
        {
            std::string strKey = "AlarmTime" + std::to_string(i + 1);
            convert.structure(pRootJson, strKey, vecInfo[i]);
        }
    }
}

void Convert::deal(Json::Object *pRootJson, Alarm::Region_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "PointNum", (int &) stInfo.nPointNum);
    convert.structure(pRootJson, "Points", stInfo.aPoint);
}

void Convert::deal(Json::Object* pRootJson, Alarm::EventSchedule_S &stSchedule, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "EventType", (int &) stSchedule.enEventType);
    convert.field(pRootJson, "Status", stSchedule.bStatus);
    convert.structure(pRootJson, stSchedule.defenseTime);
}

void Convert::deal(Json::Object* pRootJson, std::set<Alarm::EventSchedule_S> &stSchedule, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.structure(pRootJson, "EventSchedule", stSchedule);
}

void Convert::deal(Json::Object* pRootJson, Alarm::VideoLostDetection_S &stVideoLostDetection, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "ChnId", stVideoLostDetection.nChnId);
	convert.field(pRootJson, "IsEnableDetection", stVideoLostDetection.bEnable);
	convert.structure(pRootJson, stVideoLostDetection.aAlarmTime);
    convert.structure(pRootJson, "LinkageMode", stVideoLostDetection.stLinkageList);
}

void Convert::deal(Json::Object* pRootJson, std::set<Alarm::VideoLostDetection_S> &videoLostDetection, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.structure(pRootJson, "AlarmOutputInfos", videoLostDetection);
}
