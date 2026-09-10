/**
 * @file event_database_manage.h
 * @author xiejh (xiejh@kfb.cn)
 * @date 2024-10-15
 *
 * @brief 检索数据库管理：提供增删改相关接口
 */

#pragma once
#include "event_database.h"
#include "event_define.h"
// #include "VehicleDatabase.h"
#include <map>

using namespace Db;

class EventDatabaseManage : public CSingleton<EventDatabaseManage>
{

public:
    EventDatabaseManage() {};
    ~EventDatabaseManage() {};
    friend class CSingleton<EventDatabaseManage>;

    int init();
    void deinit();

    /**
     * @brief 新增普通事件
     * @param stEventInfo
     * @return int
     */
    int add(Event::Info_S stEventInfo);

    // /**
    //  * @brief 新增车辆事件
    //  * @param stVehicleInfo
    //  * @return int
    //  */
    // int add(Event::VehicleInfo_S stVehicleInfo);

    /**
     * @brief 新增人脸比对事件
     * @return int
     */
    int add(Event::FaceCompareInfo_S stFaceCompareInfo);

    /**
     * @brief 删除普通事件
     * @param stEventInfo
     * @return int
     */
    int del(Event::Info_S stEventInfo);

    /**
     * @brief 删除事件信息
     * @param stCond 查找条件
     * @param strTargetTableName 目标表名
     * @return int <0:失败
     */
    int del(Event::RetrievalCond_S &stCond, std::string strTargetTableName = std::string());

    // /**
    //  * @brief 删除车辆事件
    //  * @param stVehicleInfo
    //  * @return int
    //  */
    // int del(Event::VehicleInfo_S stVehicleInfo);

    /**
     * @brief 更新普通事件信息
     * @param stEventInfo
     * @return int
     */
    int update(Event::Info_S stEventInfo);

    // /**
    //  * @brief 更新车辆事件信息
    //  * @param stVehicleInfo
    //  * @return int
    //  */
    // int update(Event::VehicleInfo_S stVehicleInfo);

    /**
     * @brief 获取Event数据
     * @param stInfo
     * @return int <0失败
     */
    int get_itemInfo(Event::Info_S &stInfo);

    /* 查找人脸比对事件 */
    int find(Event::RetrievalCond_S stCond, std::vector<Event::FaceCompareInfo_S> &faceCompareInfos, bool bAsc = true);
    int find(Event::RetrievalCond_S stCond, std::vector<Event::FaceCompareInfo_S> &faceCompareInfos, Common::PageInfo_S &stPageInfo, bool bAsc = true);

    int set_rule(Event::RuleInfo_S stRuleInfo);
    std::vector<Event::RuleInfo_S> get_rule();

private:
    /* 移动侦测事件, 最新事件记录 */
    std::map<int, Event::Info_S> m_motionDetectMap;
};
