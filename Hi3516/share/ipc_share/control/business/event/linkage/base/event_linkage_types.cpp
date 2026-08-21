/**
 * @FilePath     : event_linkage_types.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-15 16:29:58
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-20 14:41:01
 * @Description  : 事件联动内部公共类型定义实现模块
 */

#include "event_linkage_types.h"

#include <algorithm>

#include "alarm_convert.h"
#include "convert.h"

namespace
{
/* 事件类型优先级映射 */
const std::map<Event::Type_E, int> g_event_priority_map = {
    {Event::Type_E::FACE_COMPARE_SUCCESS, 0}, /* 人脸比对成功播报优先于通用报警音 */
    {    Event::Type_E::MOTION_DETECT,  1}, /* 移动侦测事件 */
    { Event::Type_E::OCCLUSION_DETECT,  2}, /* 遮挡侦测事件 */
    {    Event::Type_E::ANOMALY_ALARM,  3}, /* 异常报警事件 */
    {      Event::Type_E::ALARM_INPUT,  4}, /* 报警输入事件 */
    {        Event::Type_E::PIR_ALARM,  5}, /* PIR报警事件 */
    {    Event::Type_E::LINE_CROSSING,  6}, /* 绊线侦测事件 */
    {        Event::Type_E::INTRUSION,  7}, /* 区域入侵事件 */
    {     Event::Type_E::ENTER_REGION,  8}, /* 进入区域事件 */
    {     Event::Type_E::LEAVE_REGION,  9}, /* 离开区域事件 */
    {    Event::Type_E::AUDIO_ANOMALY, 10}, /* 音频异常事件 */
    {Event::Type_E::AUDIO_SUDDEN_RISE, 11}, /* 音量骤升事件 */
    {Event::Type_E::AUDIO_SUDDEN_DROP, 12}, /* 音量骤降事件 */
    {     Event::Type_E::SCENE_CHANGE, 13}, /* 场景变更事件 */
    {      Event::Type_E::FACE_DETECT, 14}, /* 人脸检测事件 */
    { Event::Type_E::LOITERING_DETECT, 15}, /* 徘徊检测事件 */
    {  Event::Type_E::CROWD_GATHERING, 16}, /* 人群聚集事件 */
    {   Event::Type_E::PARKING_DETECT, 17}, /* 违停检测事件 */
    {Event::Type_E::UNATTENDED_OBJECT, 18}, /* 物品遗留事件 */
    {   Event::Type_E::OBJECT_REMOVAL, 19}, /* 物品移除事件 */
    {  Event::Type_E::PET_RECOGNITION, 20}, /* 宠物识别事件 */
    {     Event::Type_E::FACE_CAPTURE, 21}, /* 人脸抓拍事件 */
    {   Event::Type_E::FACE_COMPARE_FAIL, 23}, /* 人脸比对失败事件 */
#if defined(SCENE_INTELLIGENCE) || CAP_AI_GARBAGE_DETECT
    { Event::Type_E::GARBAGE_EXPOSURE, 70}, /* 垃圾暴露识别事件 */
    { Event::Type_E::GARBAGE_OVERFLOW, 71}, /* 垃圾满溢识别事件 */
#endif
#if CAP_AI_PEOPLE_STATISTICS
    {Event::Type_E::PEOPLE_FLOW_STATISTICS, 72}, /* 人流统计事件 */
    {Event::Type_E::PEOPLE_DENSITY_DETECTION, 73}, /* 人员密度检测事件 */
    {Event::Type_E::PEOPLE_FLOW_STAY_NORMAL, 74}, /* 人流统计普通报警事件 */
    {Event::Type_E::PEOPLE_FLOW_STAY_MEDIUM, 75}, /* 人流统计中度报警事件 */
    {Event::Type_E::PEOPLE_FLOW_STAY_SEVERE, 76}, /* 人流统计严重报警事件 */
    {Event::Type_E::PEOPLE_DENSITY_NORMAL, 77}, /* 人员密度普通报警事件 */
    {Event::Type_E::PEOPLE_DENSITY_MEDIUM, 78}, /* 人员密度中度报警事件 */
    {Event::Type_E::PEOPLE_DENSITY_SEVERE, 79}, /* 人员密度严重报警事件 */
#endif
};

/* 联动类型优先级映射 */
const std::map<LinkageType_E, int> g_linkage_priority_map = {
    {LinkageType_E::RECORD,          0}, /* 录像联动 */
    {LinkageType_E::CAPTURE,         1}, /* 抓图联动 */
    {LinkageType_E::UPLOAD_TOCENTER, 2}, /* 上传中心联动 */
    {LinkageType_E::EMAIL,           3}, /* 邮件通知联动 */
    {LinkageType_E::UPLOAD_SD_CARD,  4}, /* 上传SD卡联动 */
    {LinkageType_E::FLASHING_LIGHT,  5}, /* 闪光报警灯联动 */
    {LinkageType_E::ALARM_IO,        6}, /* 报警IO联动 */
    {LinkageType_E::LOG,             7}, /* 日志联动 */
    {LinkageType_E::SOUND,           8}, /* 声音联动 */
    {LinkageType_E::NONE,            9}, /* 无联动 */
};

/**
 * @brief   : 读写属性键值表
 * @param    {Json::Object} *pRootJson json节点
 * @param    {std::map<std::string, std::string>} &mapAttrs 属性表
 * @param    {bool} bOutStruct true：json转结构 false：结构转json
 */
void deal_attr_map(Json::Object *pRootJson, std::map<std::string, std::string> &mapAttrs, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    if (bOutStruct)
    {
        mapAttrs.clear();
        for (Json::Object *pItem = pRootJson->child; pItem != nullptr; pItem = pItem->next)
        {
            if (!pItem->string)
            {
                continue;
            }

            std::string strValue;
            if (!Json::Value::get(pItem, strValue))
            {
                continue;
            }

            mapAttrs[pItem->string] = strValue;
        }
        return;
    }

    for (const auto &item : mapAttrs)
    {
        Json::add(pRootJson, item.first, item.second);
    }
}
} // namespace

bool LinkageTask_S::operator<(const LinkageTask_S &other) const
{
    if (nPriority == other.nPriority)
    {
        return llTimestamp > other.llTimestamp;
    }

    return nPriority > other.nPriority;
}

int get_linkage_task_priority(Event::Type_E enEventType, LinkageType_E enLinkageType)
{
    constexpr int EVENT_WEIGHT = 100;

    auto itEvent = g_event_priority_map.find(enEventType);
    auto itLinkage = g_linkage_priority_map.find(enLinkageType);
    if (itEvent == g_event_priority_map.end() || itLinkage == g_linkage_priority_map.end())
    {
        return INT_MAX;
    }

    return itEvent->second * EVENT_WEIGHT + itLinkage->second;
}

bool linkage_list_contains(const Alarm::LinkageList_S &stLinkageList, Alarm::LinkageType_E enLinkageType)
{
    return std::find(stLinkageList.tradition.begin(), stLinkageList.tradition.end(), static_cast<int>(enLinkageType)) !=
           stLinkageList.tradition.end();
}

void Convert::deal(Json::Object *pRootJson, LinkageAttrMatch_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    int nOp = static_cast<int>(stInfo.enOp);

    convert.field(pRootJson, "Key", stInfo.strKey);
    convert.field(pRootJson, "Op", nOp);
    convert.field(pRootJson, "Values", stInfo.vecValues);

    if (bOutStruct)
    {
        stInfo.enOp = static_cast<LinkageMatchOp_E>(nOp);
    }
}

void Convert::deal(Json::Object *pRootJson, std::vector<LinkageAttrMatch_S> &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "AttrMatches", stInfo);
}

void Convert::deal(Json::Object *pRootJson, LinkageRule_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    int nEventType = static_cast<int>(stInfo.enEventType);

    convert.field(pRootJson, "RuleId", stInfo.nRuleId);
    convert.field(pRootJson, "Priority", stInfo.nPriority);
    convert.field(pRootJson, "EventType", nEventType);
    convert.field(pRootJson, "ChnId", stInfo.nChnId);
    convert.field(pRootJson, "Phase", stInfo.nPhase);
    convert.structure(pRootJson, "AttrMatches", stInfo.vecAttrMatches);
    convert.structure(pRootJson, "LinkageList", stInfo.stLinkageList);

    Json::Object *pAttrs = nullptr;
    if (bOutStruct)
    {
        stInfo.enEventType = static_cast<Event::Type_E>(nEventType);
    }

    pAttrs = Json::get(pRootJson, "Attrs");
    if (!pAttrs && !bOutStruct)
    {
        pAttrs = Json::init();
        Json::add(pRootJson, "Attrs", pAttrs);
    }

    /* 兼容保留字段，避免历史配置中出现 Attrs 时解析失败 */
    std::map<std::string, std::string> mapAttrs;
    deal_attr_map(pAttrs, mapAttrs, bOutStruct);
}

void Convert::deal(Json::Object *pRootJson, std::vector<LinkageRule_S> &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "Rules", stInfo);
}

void Convert::deal(Json::Object *pRootJson, LinkagePolicySet_S &stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Version", stInfo.nVersion);
    convert.structure(pRootJson, "Rules", stInfo.vecRules);
}
