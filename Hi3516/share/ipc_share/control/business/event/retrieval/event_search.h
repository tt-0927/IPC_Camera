/**
 * @file EventSearch.h
 * @author xiejh (xiejh@kfb.cn)
 * @date 2024-10-15
 * 
 * @brief 检索数据库管理：提供检索相关接口
 */

#pragma once

// #include "VehicleDatabase.h"
#include "capture_define.h"
#include "capture_database.h"
#include "event_database.h"
#include "common_define.h"
#include "Singleton.h"

class EventSearch : public CSingleton<EventSearch>
{
    EventSearch() = default;
public:
    ~EventSearch() = default;
    friend class CSingleton<EventSearch>;

    int init();
    void deinit();
    
    /**
     * @brief 通过通道检索事件
     * @param chnIds 
     * @param EventInfos 
     * @return int 
     */
    int searchByChnId(const std::vector<int>& chnIds, std::vector<Event::Info_S> &EventInfos);


    /**
     * @brief 通过事件类型检索事件
     * @param stCond 
     * @param EventInfos 
     * @return int 
     */
    int searchByEventType(Event::RetrievalCond_S &stCond, std::vector<Event::Info_S> &EventInfos);
    int searchByEventType(Event::RetrievalCond_S &stCond, std::vector<Event::Info_S> &EventInfos, Common::PageInfo_S &stPageInfo, bool bAsc = true, bool bImage = false);

    
    /**
     * @brief 通过日期检索事件
     * @param date 
     * @param EventInfos 
     * @return int 
     */
    int searchByDate(std::string startdate, std::string enddate, std::vector<Event::Info_S> &EventInfos);

    
    /**
     * @brief 通过时间检索事件
     * @param starttime 
     * @param endtime 
     * @param EventInfos 
     * @return int 
     */
    int searchByTime(std::string starttime, std::string endtime, std::vector<Event::Info_S> &EventInfos);
    
    
    /**
     * @brief 图片检索
     * @param stCond 
     * @param EventInfos 
     * @return int 
     */
    int searchByImageType(Event::RetrievalCond_S &stCond, std::vector<Capture_NS::CaptureInfo_S> &CaptureInfos, Common::PageInfo_S &stPageInfo);


    /**
     * @brief 通过车辆信息检索事件
     * @param plateregion 
     * @param plateserial 
     * @param VehicleInfos 
     * @return int 
     */
    int searchByVehicle(Event::RetrievalCond_S &stCond, std::vector<Event::VehicleInfo_S> &VehicleInfos);


    /**
     * @brief 通过视频类型检索事件
     * @param stCond 
     * @param EventInfos 
     * @return int 
     */
    int searchByRecordType(Event::RetrievalCond_S &stCond, std::vector<Event::Info_S> &EventInfos);
};
