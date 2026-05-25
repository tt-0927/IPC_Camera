/**
 * @FilePath     : event_linkage_types.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-15 16:29:58
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-05-07 15:28:43
 * @Description  : 事件联动内部公共类型定义模块
 */

#pragma once

#include <climits>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Json.h"
#include "alarm_define.h"
#include "event_define.h"
#include "event_tvsdk_payload.h"

/**
 * @brief   : 联动类型枚举
 */
enum class LinkageType_E
{
    NONE = 0,            /* 无联动 */
    EMAIL = 1,           /* 发送邮件 */
    UPLOAD_TOCENTER = 2, /* 上传中心 */
    UPLOAD_SD_CARD = 3,  /* 上传SD卡 */
    SOUND = 4,           /* 声音联动 */
    FLASHING_LIGHT = 5,  /* 闪光报警灯 */
    RECORD = 6,          /* 录像联动 */
    CAPTURE = 7,         /* 抓图联动 */
    ALARM_IO = 8,        /* 报警IO联动 */
    LOG = 9,             /* 日志-onvif */
};

/**
 * @brief   : 事件触发上下文
 */
struct EventTriggerContext_S
{
    /* 触发事件类型 */
    Event::Type_E enEventType = Event::Type_E::UNKNOWN;
    /* 是否为事件结束阶段 */
    bool bEventEnded = false;
    /* 触发通道号 */
    int nChnId = 0;
    /* 触发时间戳，单位毫秒 */
    long long llTimestamp = 0;
    /* 扩展属性键值对，如 result/level/source/rule_id 等 */
    std::map<std::string, std::string> mapAttrs;
    /* TVSDK 推送扩展负载，承载图片、统计目标等大对象并保持所有权 */
    std::shared_ptr<const EventTvSdkPayload_S> pTvSdkPayload;
};

/**
 * @brief   : 根据事件类型获取对应的 TVSDK 负载类型
 * @param    {Event::Type_E} enEventType 事件类型
 * @return   {EventTvSdkPayloadType_E} TVSDK 负载类型，未知事件返回 NONE
 */
inline EventTvSdkPayloadType_E get_tvsdk_payload_type(Event::Type_E enEventType)
{
    switch (enEventType)
    {
    /* 普通事件 */
    case Event::Type_E::MOTION_DETECT:
    case Event::Type_E::OCCLUSION_DETECT:
    case Event::Type_E::ANOMALY_ALARM:
    case Event::Type_E::ALARM_INPUT:
    case Event::Type_E::PIR_ALARM:
        return EventTvSdkPayloadType_E::BASIC;

    /* 周界规则事件 */
    case Event::Type_E::LINE_CROSSING:
    case Event::Type_E::INTRUSION:
    case Event::Type_E::ENTER_REGION:
    case Event::Type_E::LEAVE_REGION:
    case Event::Type_E::UNATTENDED_OBJECT:
    case Event::Type_E::OBJECT_REMOVAL:
        return EventTvSdkPayloadType_E::RULE;

    /* AI 检测类事件 */
    case Event::Type_E::FACE_DETECT:
    case Event::Type_E::PET_RECOGNITION:
    case Event::Type_E::LOITERING_DETECT:
    case Event::Type_E::PARKING_DETECT:
    case Event::Type_E::GARBAGE_EXPOSURE:
    case Event::Type_E::GARBAGE_OVERFLOW:
    case Event::Type_E::SCENE_CHANGE:
    case Event::Type_E::AUDIO_ANOMALY:
    case Event::Type_E::AUDIO_SUDDEN_RISE:
    case Event::Type_E::AUDIO_SUDDEN_DROP:
        return EventTvSdkPayloadType_E::AI_OBJECT;

    /* 统计类事件 */
#if CAP_AI_PEOPLE_STATISTICS
    case Event::Type_E::PEOPLE_FLOW_STATISTICS:
    case Event::Type_E::PEOPLE_DENSITY_DETECTION:
    case Event::Type_E::PEOPLE_FLOW_STAY_NORMAL:
    case Event::Type_E::PEOPLE_FLOW_STAY_MEDIUM:
    case Event::Type_E::PEOPLE_FLOW_STAY_SEVERE:
    case Event::Type_E::PEOPLE_DENSITY_NORMAL:
    case Event::Type_E::PEOPLE_DENSITY_MEDIUM:
    case Event::Type_E::PEOPLE_DENSITY_SEVERE:
        return EventTvSdkPayloadType_E::STATISTICS;
#endif
    default:
        return EventTvSdkPayloadType_E::NONE;
    }
}

/**
 * @brief   : 联动属性匹配操作符
 */
enum class LinkageMatchOp_E
{
    ANY = 0,
    EQ,
    IN,
};

/**
 * @brief   : 联动属性匹配条件
 */
struct LinkageAttrMatch_S
{
    /* 属性键 */
    std::string strKey;
    /* 匹配操作符 */
    LinkageMatchOp_E enOp = LinkageMatchOp_E::ANY;
    /* 匹配值集合 */
    std::vector<std::string> vecValues;
};

/**
 * @brief   : 联动规则定义
 */
struct LinkageRule_S
{
    /* 规则编号 */
    int nRuleId = -1;
    /* 规则优先级，数值越小越优先 */
    int nPriority = 1000;
    /* 规则绑定的事件类型 */
    Event::Type_E enEventType = Event::Type_E::UNKNOWN;
    /* 规则绑定的通道号，-1 表示任意通道 */
    int nChnId = -1;
    /* 规则绑定的阶段，-1 任意，0 结束，1 开始 */
    int nPhase = -1;
    /* 属性匹配条件列表 */
    std::vector<LinkageAttrMatch_S> vecAttrMatches;
    /* 命中规则后使用的联动列表 */
    Alarm::LinkageList_S stLinkageList;
};

/**
 * @brief   : 联动规则集合
 */
struct LinkagePolicySet_S
{
    /* 策略格式版本 */
    int nVersion = 1;
    /* 联动规则集合 */
    std::vector<LinkageRule_S> vecRules;
};

/**
 * @brief   : 联动解析结果
 */
struct ResolvedLinkagePlan_S
{
    /* 当前触发上下文快照 */
    EventTriggerContext_S stContext;
    /* 当前事件信息快照 */
    Event::Info_S stEventInfo;

    /* 是否命中扩展规则 */
    bool bUseExtendedRule = false;
    /* 命中的规则编号，未命中时为 -1 */
    int nMatchedRuleId = -1;

    /* 最终生效的联动列表 */
    Alarm::LinkageList_S stLinkageList;
    /* 当前算法总开关配置 */
    Event::AlgorithmConfig_S stAlgorithmConfig;

    /* 是否启用上传 SD 卡 */
    bool bUploadSdCard = false;
    /* 是否启用发送邮件 */
    bool bSendEmail = false;
    /* 是否启用声音联动 */
    bool bSound = false;
    /* 是否启用闪光报警灯 */
    bool bFlashingLightAlarm = false;
    /* 是否启用上传中心 */
    bool bUploadToCenter = false;
};

/**
 * @brief   : 联动异步任务快照
 */
struct LinkageTask_S
{
    /* 任务绑定的事件触发上下文 */
    EventTriggerContext_S stContext;
    /* 任务绑定的事件信息 */
    Event::Info_S stEventInfo;

    /* 异步联动类型 */
    LinkageType_E enLinkageType = LinkageType_E::NONE;
    /* 执行优先级，数值越小优先级越高 */
    int nPriority = INT_MAX;
    /* 任务入队时间戳 */
    long long llTimestamp = 0;

    /* 报警输出联动的 IO 编号列表 */
    std::vector<int> vecAlarmOutputNum;
    /* 邮件联动是否需要等待事件抓图 */
    bool bUploadSdCard = false;

    /**
     * @brief   : 优先队列比较运算符
     * @param    {LinkageTask_S} &other 对比任务
     * @return   {bool} true：当前任务优先级更低
     */
    bool operator<(const LinkageTask_S &other) const;
};

/**
 * @brief   : 计算异步联动任务优先级
 * @param    {Event::Type_E} enEventType 事件类型
 * @param    {LinkageType_E} enLinkageType 联动类型
 * @return   {int} 数值越小优先级越高
 */
int get_linkage_task_priority(Event::Type_E enEventType, LinkageType_E enLinkageType);

/**
 * @brief   : 判断常规联动列表中是否包含指定类型
 * @param    {Alarm::LinkageList_S} &stLinkageList 联动列表
 * @param    {Alarm::LinkageType_E} enLinkageType 联动类型
 * @return   {bool} true：包含 false：不包含
 */
bool linkage_list_contains(const Alarm::LinkageList_S &stLinkageList, Alarm::LinkageType_E enLinkageType);

namespace Convert
{
void deal(Json::Object *pRootJson, LinkageAttrMatch_S &stInfo, bool bOutStruct);
void deal(Json::Object *pRootJson, std::vector<LinkageAttrMatch_S> &stInfo, bool bOutStruct);
void deal(Json::Object *pRootJson, LinkageRule_S &stInfo, bool bOutStruct);
void deal(Json::Object *pRootJson, std::vector<LinkageRule_S> &stInfo, bool bOutStruct);
void deal(Json::Object *pRootJson, LinkagePolicySet_S &stInfo, bool bOutStruct);
}
