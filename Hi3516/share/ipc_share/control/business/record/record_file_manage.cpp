#include "record_file_manage.h"

#include "IpcRet.h"
#include "record_file_database.h"
#include "m3u8.h"

#include <sstream>
#include <iomanip> // Include for std::get_time and std::put_time
#include <ctime>   // Include for std::tm
#include <regex>
#include <cmath>
#include <sys/time.h>
#include <sys/stat.h>
#include <optional>

#include "event_manage.h"
#include "log_handler.h"
#include "event_database_manage.h"
#include "record_ctrl.h"
#include "storage_manage.h"
#include "time_utils.h"
#include "event_linkage.h"
#include "event_define.h"

using namespace Db;


int RecordFileManage::init()
{
    m_bRun.store(true, std::memory_order_release);
    std::thread tid;
    tid = std::thread(&RecordFileManage::record_file_manage_thread, this);
    tid.detach();

    return 0;
}

void RecordFileManage::deinit()
{
    m_bRun.store(false, std::memory_order_release);
    return;
}

int RecordFileManage::add(Record_NS::FileInfo_S stnfo)
{
    std::lock_guard<std::mutex> lock(m_eventMutex);
    return RecordFileDatabase::instance()->add(stnfo);
}
int RecordFileManage::add(Record_NS::TsFileInfo_S stTsFileInfo)
{
    int nVideoType = -1;
    /* 处理ts文件 */
    deal_tsFile(stTsFileInfo);
    /* 处理事件文件 */
    deal_eventFile(stTsFileInfo, nVideoType);
    if (CRecordCtrl::instance()->get_RecordScheduleType() == 2)
    {
        Event::Info_S stEventInfo;
        CRecordCtrl::instance()->get_event_record(stEventInfo);
        stTsFileInfo.nType = (int)stEventInfo.enType;

        std::string date_str;
        date_str = TimeUtils_NS::get_currentDate();
        std::string strFullPath = stTsFileInfo.path + "/normal_" + date_str + ".m3u8";
        CRecordCtrl::instance()->set_event_ts_info(stTsFileInfo.nSize, strFullPath);
    }
    else 
    {
        stTsFileInfo.nType = nVideoType; 
    }

    std::lock_guard<std::mutex> lock(m_eventMutex); 
    return RecordFileDatabase::instance()->add(stTsFileInfo);
}

int RecordFileManage::del(Record_NS::FileInfo_S stnfo)
{
    Item item;
    item.push_back(Element(RECORD_FILE_FIELD_CHN_ID, stnfo.nChnId));
    std::lock_guard<std::mutex> lock(m_eventMutex);
    return RecordFileDatabase::instance()->del(item);
}

int RecordFileManage::del(Event::RetrievalCond_S &stCond, std::string strTargetTableName)
{
    std::lock_guard<std::mutex> lock(m_eventMutex);
    MatchMethods methods;

    /* 多通道查询 */
    if (stCond.nChnIds.size() > 0)
    {
        for (auto it = stCond.nChnIds.begin(); it != stCond.nChnIds.end(); ++it)
        {
            methods.push_back(MatchMethod(Element(RECORD_FILE_FIELD_CHN_ID, std::to_string(*it)), FIND_CRITERION_EQ, FIND_CRITERION_OR_P));
        }
        methods.back().enAndOr = FIND_CRITERION_AND;
    }
    if(strTargetTableName == RECORD_FILE_TABLE_NAME)
    {
        /* 表record_file_manage的字段create_time格式为 2025-09-12 18:25:33 */
        if (!stCond.strTime.empty()) 
        {
            methods.push_back(MatchMethod(Element(RECORD_FILE_FIELD_CREATE_TIME, stCond.strTime), FIND_CRITERION_GE, FIND_CRITERION_AND));
        }
        if(!stCond.strFilename.empty())
        {
            methods.push_back(MatchMethod(Element(RECORD_FILE_FIELD_FILENAME, stCond.strFilename), FIND_CRITERION_NE, FIND_CRITERION_AND));
        }
    }
    else 
    {
        /* 其他录制ts文件表的字段create_time格式为 18:25:33 */
        if (!stCond.strTime.empty()) 
        {
            methods.push_back(MatchMethod(Element(RECORD_FILE_FIELD_CREATE_TIME, stCond.strTime), FIND_CRITERION_GE, FIND_CRITERION_AND));
        }
    }


    if (methods.size() != 0)
    {
        MatchMethod &lastMethod = methods.back();
        lastMethod.enAndOr = FIND_CRITERION_NONE;
    }
 
    return RecordFileDatabase::instance()->del(methods, strTargetTableName);
}

int RecordFileManage::update(Record_NS::UpdateInfo_S stUpdateInfo)
{
    std::lock_guard<std::mutex> lock(m_eventMutex);
    std::vector<Record_NS::FileInfo_S> infos;
    RecordFileDatabase::instance()->find(Element(DB_COMMON_FIELD_ID, stUpdateInfo.nId), infos);
    if (infos.size() == 0)
    {
        dlog_error("不存在");
        return -1;
    }

    Item item;
    item.push_back(Element(RECORD_FILE_FIELD_CHN_ID, stUpdateInfo.stNewFileInfo.nChnId));
    item.push_back(Element(RECORD_FILE_FIELD_PATH, stUpdateInfo.stNewFileInfo.path));
    item.push_back(Element(RECORD_FILE_FIELD_FILENAME, stUpdateInfo.stNewFileInfo.filename));
    item.push_back(Element(RECORD_FILE_FIELD_SIZE, stUpdateInfo.stNewFileInfo.nSize));
    item.push_back(Element(RECORD_FILE_FIELD_TYPE, stUpdateInfo.stNewFileInfo.nType));
    item.push_back(Element(RECORD_FILE_FIELD_IS_LOCK, stUpdateInfo.stNewFileInfo.bLock));
    item.push_back(Element(RECORD_FILE_FIELD_STATUS, stUpdateInfo.stNewFileInfo.nStatus));
    item.push_back(Element(RECORD_FILE_FIELD_CREATE_TIME, stUpdateInfo.stNewFileInfo.createTime));
    item.push_back(Element(RECORD_FILE_FIELD_MODIFY_TIME, stUpdateInfo.stNewFileInfo.modifyTime));
    item.push_back(Element(RECORD_FILE_FIELD_DURATION, stUpdateInfo.stNewFileInfo.nDuration));

    MatchMethods methods;
    methods.push_back(MatchMethod(Element(DB_COMMON_FIELD_ID, stUpdateInfo.nId), FIND_CRITERION_EQ));
    return RecordFileDatabase::instance()->update(item, methods);
}

int RecordFileManage::update(Record_NS::TsFileInfo_S stTsFileInfo, std::string strTargetTableName)
{
    Item item;
    item.push_back(Element(RECORD_FILE_FIELD_TYPE, stTsFileInfo.nType));
    MatchMethods methods;
    methods.push_back(MatchMethod(Element(RECORD_FILE_FIELD_FILENAME, stTsFileInfo.filename), FIND_CRITERION_EQ));
    return RecordFileDatabase::instance()->update(item, methods, strTargetTableName);
}

int RecordFileManage::find(Record_NS::Find_S stFind, std::vector<Record_NS::FileInfo_S>& infos)
{
    MatchMethods methods;
    if (stFind.nChnId >= 0)
    {
        methods.push_back(MatchMethod(Element(RECORD_FILE_FIELD_CHN_ID, stFind.nChnId), FIND_CRITERION_EQ, FIND_CRITERION_AND));
    }
    if (stFind.nType >= 0)
    {
        methods.push_back(MatchMethod(Element(RECORD_FILE_FIELD_TYPE, stFind.nType), FIND_CRITERION_EQ, FIND_CRITERION_AND));
    }
    if (!stFind.year.empty())
    {
        std::string key= "strftime('%Y'," + std::string(RECORD_FILE_FIELD_CREATE_TIME) + ")";
        methods.push_back(MatchMethod(Element(key, stFind.year), FIND_CRITERION_EQ, FIND_CRITERION_AND));
    }
    if (!stFind.month.empty())
    {
        std::string key= "strftime('%m'," + std::string(RECORD_FILE_FIELD_CREATE_TIME) + ")";
        methods.push_back(MatchMethod(Element(key, stFind.month), FIND_CRITERION_EQ, FIND_CRITERION_AND));
    }
    if (!stFind.date.empty())
    {
        methods.push_back(MatchMethod(Element(RECORD_FILE_FIELD_CREATE_TIME, stFind.date), FIND_CRITERION_LIKE, FIND_CRITERION_AND));
    }
    if (!stFind.startTime.empty())
    {
        methods.push_back(MatchMethod(Element(RECORD_FILE_FIELD_CREATE_TIME, stFind.startTime), FIND_CRITERION_EQ, FIND_CRITERION_AND));
    }
    if (!stFind.videoTime.empty())
    {
        methods.push_back(MatchMethod(Element(RECORD_FILE_FIELD_CREATE_TIME, stFind.videoTime), FIND_CRITERION_IE, FIND_CRITERION_AND));
        methods.push_back(MatchMethod(Element(RECORD_FILE_FIELD_MODIFY_TIME, stFind.videoTime), FIND_CRITERION_GE, FIND_CRITERION_AND));
    }

    if (!stFind.startTime.empty())
    {
        methods.push_back(MatchMethod(Element(RECORD_FILE_FIELD_CREATE_TIME, stFind.startTime), FIND_CRITERION_EQ, FIND_CRITERION_AND));
    }
    
    if (!stFind.endTime.empty())
    {
        methods.push_back(MatchMethod(Element(RECORD_FILE_FIELD_MODIFY_TIME, stFind.endTime), FIND_CRITERION_EQ, FIND_CRITERION_AND));
    }
    MatchMethod &lastMethod = methods.back();
    lastMethod.enAndOr = FIND_CRITERION_NONE;
    std::lock_guard<std::mutex> lock(m_eventMutex);
    return RecordFileDatabase::instance()->find(methods, infos);
}

int RecordFileManage::find(Record_NS::Find_S stFind, std::vector<Record_NS::FindResult_S> &outInfos)
{
    std::vector<::Record_NS::FileInfo_S> infos;
    int nRet = find(stFind, infos);
    if (nRet < 0)
    {
        return nRet;
    }

    std::map<int, Record_NS::FindResult_S> groupedResults;
    for (auto &info : infos)
    {
        info.createTime.resize(strlen("2024-10-30")); // 调整日期格式长度
        auto& result = groupedResults[info.nChnId];   // 获取当前 ChnId 的记录
        result.nChnId = info.nChnId;                 // 更新 ChnId
        result.dates.push_back(info.createTime);     // 添加日期

        /* 按日期搜索才返回录制文件 */
        if (!stFind.date.empty())
        {
            result.filename = info.path + "/" + info.filename;
        }
    }    
    // 将 map 中的结果转移到输出容器
    for (const auto& [chnId, result] : groupedResults)
    {
        outInfos.push_back(result);
    }
    return 0;
}

int RecordFileManage::find(Record_NS::TsFind_S stTsFind, Record_NS::TsFileInfo_S &stTsFileInfo)
{
    stTsFind.date.resize(strlen("YYYY-MM-DD"));
    std::string cmd;
    std::vector<Record_NS::TsFileInfo_S> infos;

    /* 查询录制文件 */
    if (stTsFind.nId < 0)
    {
        cmd = "select * from \"" + stTsFind.date + 
            "\" where "  +  
            std::string(RECORD_FILE_FIELD_CHN_ID) + " = '" + std::to_string(stTsFind.nChnId) + 
            "' and " + 
            std::string(RECORD_FILE_FIELD_CREATE_TIME) + " <= '" + stTsFind.date + ' ' +  stTsFind.time + 
            "' and "
            "time(" + std::string(RECORD_FILE_FIELD_CREATE_TIME) + ", '+10 seconds') > '" + stTsFind.time + 
            "' order by "+ std::string(RECORD_FILE_FIELD_CREATE_TIME) +" asc limit 1;";
    }
    else
    {
        if (stTsFind.nType == -1)
        {
            cmd = "select * from \"" + stTsFind.date + 
                "\" where "  +  
                std::string(DB_COMMON_FIELD_ID) + " < '" + std::to_string(stTsFind.nId) + 
                "' and " +
                std::string(RECORD_FILE_FIELD_CHN_ID) + " = '" + std::to_string(stTsFind.nChnId) +
                 "' and " +
                std::string(RECORD_FILE_FIELD_FILE_INDEX) + " = '" + std::to_string(stTsFind.nIndex) +
                "' order by "+ std::string(DB_COMMON_FIELD_ID) +" desc limit 1;";
        }
        else if (stTsFind.nType == 1)
        {
            cmd = "select * from \"" + stTsFind.date + 
                "\" where "  +  
                std::string(DB_COMMON_FIELD_ID) + " > '" + std::to_string(stTsFind.nId) + 
                "' and " +
                std::string(RECORD_FILE_FIELD_CHN_ID) + " = '" + std::to_string(stTsFind.nChnId) +
                 "' and " +
                std::string(RECORD_FILE_FIELD_FILE_INDEX) + " = '" + std::to_string(stTsFind.nIndex) +
                "' order by "+ std::string(DB_COMMON_FIELD_ID) +" asc limit 1;";
        }
    }
    std::lock_guard<std::mutex> lock(m_eventMutex);
    RecordFileDatabase::instance()->find(cmd, infos);
    if (infos.size() == 0)
    {
        return -1;
    }
    stTsFileInfo = infos[0];
    return 0;
}

/* 获取表中有多少条ts文件信息 */
int RecordFileManage::getTableDataCount(Event::RetrievalCond_S &stCond, std::string strTargetTableName)
{
    MatchMethods methods;
    int nCount = 0;

    /* 多通道查询 */
    if (stCond.nChnIds.size() > 0)
    {
        for (auto it = stCond.nChnIds.begin(); it != stCond.nChnIds.end(); ++it)
        {
            methods.push_back(MatchMethod(Element(RECORD_FILE_FIELD_CHN_ID, std::to_string(*it)), FIND_CRITERION_EQ, FIND_CRITERION_OR_P));
        }
        methods.back().enAndOr = FIND_CRITERION_AND;
    }
    /* 开始时间 */
    if (!stCond.strStartTime.empty())
    {
        methods.push_back(MatchMethod(Element(RECORD_FILE_FIELD_CREATE_TIME, stCond.strStartTime), FIND_CRITERION_GE, FIND_CRITERION_AND));
    }
    /* 结束时间 */
    if (!stCond.strEndTime.empty()) 
    {
        methods.push_back(MatchMethod(Element(RECORD_FILE_FIELD_CREATE_TIME, stCond.strEndTime), FIND_CRITERION_IE, FIND_CRITERION_AND));
    }

    if (stCond.nVideoType == 1)
    {
        /* 数据库中录制文件类型：-1为普通的定时录制文件，其他为各事件类型录制文件 */
        methods.push_back(MatchMethod(Element(RECORD_FILE_FIELD_TYPE, (int)Event::Type::UNKNOWN), FIND_CRITERION_EQ, FIND_CRITERION_AND));
    }
    else if (stCond.nVideoType == 2)
    {
        /* 数据库中录制文件类型：-1为普通的定时录制文件，其他为各事件类型录制文件 */
        methods.push_back(MatchMethod(Element(RECORD_FILE_FIELD_TYPE, (int)Event::Type::UNKNOWN), FIND_CRITERION_NE, FIND_CRITERION_AND));
    }

    /* 结束条件 */
    if (methods.size() != 0)
    {
        MatchMethod &lastMethod = methods.back();
        lastMethod.enAndOr = FIND_CRITERION_NONE;
    }

    RecordFileDatabase::instance()->get_subDataCount(methods, nCount, DB_COMMON_FIELD_ID, strTargetTableName);

    return nCount;
}

int RecordFileManage::getTablePageInfo(Event::RetrievalCond_S &stCond, Common::PageInfo_S &stPageInfo, std::string strTargetTableName)
{
    MatchMethods methods;

    /* 多通道查询 */
    if (stCond.nChnIds.size() > 0)
    {
        for (auto it = stCond.nChnIds.begin(); it != stCond.nChnIds.end(); ++it)
        {
            methods.push_back(MatchMethod(Element(RECORD_FILE_FIELD_CHN_ID, std::to_string(*it)), FIND_CRITERION_EQ, FIND_CRITERION_OR_P));
        }
        methods.back().enAndOr = FIND_CRITERION_AND;
    }
    /* 开始时间 */
    if (!stCond.strStartTime.empty())
    {
        methods.push_back(MatchMethod(Element(RECORD_FILE_FIELD_CREATE_TIME, stCond.strStartTime), FIND_CRITERION_GE, FIND_CRITERION_AND));
    }
    /* 结束时间 */
    if (!stCond.strEndTime.empty()) 
    {
        methods.push_back(MatchMethod(Element(RECORD_FILE_FIELD_CREATE_TIME, stCond.strEndTime), FIND_CRITERION_IE, FIND_CRITERION_AND));
    }

    if (stCond.nVideoType == 1)
    {
        /* 数据库中录制文件类型：-1为普通的定时录制文件，其他为各事件类型录制文件 */
        methods.push_back(MatchMethod(Element(RECORD_FILE_FIELD_TYPE, (int)Event::Type::UNKNOWN), FIND_CRITERION_EQ, FIND_CRITERION_AND));
    }
    else if (stCond.nVideoType == 2)
    {
        /* 数据库中录制文件类型：-1为普通的定时录制文件，其他为各事件类型录制文件 */
        methods.push_back(MatchMethod(Element(RECORD_FILE_FIELD_TYPE, (int)Event::Type::UNKNOWN), FIND_CRITERION_NE, FIND_CRITERION_AND));
    }

    /* 结束条件 */
    if (methods.size() != 0)
    {
        MatchMethod &lastMethod = methods.back();
        lastMethod.enAndOr = FIND_CRITERION_NONE;
    }

    /* 带页数 */
    if (stPageInfo.nCurPage != -1)
    {
        /* 总个数, 要放在前面 */
        int nCount = -1;
        RecordFileDatabase::instance()->get_subDataCount(methods, nCount, DB_COMMON_FIELD_ID, strTargetTableName);
        stPageInfo.nDataTotal = nCount;
        /* 每页数据个数,默认20 */
        stPageInfo.nPageSize = stPageInfo.nPageSize == 0 ? 20 : stPageInfo.nPageSize;
        stPageInfo.nPageTotal = (stPageInfo.nDataTotal + stPageInfo.nPageSize - 1) / stPageInfo.nPageSize;
    }
    else
    {
        MatchMethod &lastMethod = methods.back();
        lastMethod.enAndOr = FIND_CRITERION_NONE;
        stPageInfo.nCurPage = 1;
        stPageInfo.nPageTotal = 1;
        int nCount = 0;
        RecordFileDatabase::instance()->get_subDataCount(methods, nCount, DB_COMMON_FIELD_ID, strTargetTableName);
        stPageInfo.nDataTotal = nCount;
    }

    return 0;
}

/* ts文件检索 */
int RecordFileManage::searchByRecordTs(Event::RetrievalCond_S &stCond, std::vector<::Record_NS::TsFileInfo_S> &TsFileInfos, Common::PageInfo_S &stPageInfo, std::string strTargetTableName)
{
    MatchMethods methods;

    /* 多通道查询 */
    if (stCond.nChnIds.size() > 0)
    {
        for (auto it = stCond.nChnIds.begin(); it != stCond.nChnIds.end(); ++it)
        {
            methods.push_back(MatchMethod(Element(RECORD_FILE_FIELD_CHN_ID, std::to_string(*it)), FIND_CRITERION_EQ, FIND_CRITERION_OR_P));
        }
        methods.back().enAndOr = FIND_CRITERION_AND;
    }

    if (!stCond.strStartTime.empty())
    {
        methods.push_back(MatchMethod(Element(RECORD_FILE_FIELD_CREATE_TIME, stCond.strStartTime), FIND_CRITERION_GE, FIND_CRITERION_AND));
    }

    if (!stCond.strEndTime.empty()) 
    {
        methods.push_back(MatchMethod(Element(RECORD_FILE_FIELD_CREATE_TIME, stCond.strEndTime), FIND_CRITERION_IE, FIND_CRITERION_AND));
    }

    if (stCond.nVideoType == 1)
    {
        /* 数据库中录制文件类型：-1为普通的定时录制文件，其他为各事件类型录制文件 */
        methods.push_back(MatchMethod(Element(RECORD_FILE_FIELD_TYPE, (int)Event::Type::UNKNOWN), FIND_CRITERION_EQ, FIND_CRITERION_AND));
    }
    else if (stCond.nVideoType == 2)
    {
        /* 数据库中录制文件类型：-1为普通的定时录制文件，其他为各事件类型录制文件 */
        methods.push_back(MatchMethod(Element(RECORD_FILE_FIELD_TYPE, (int)Event::Type::UNKNOWN), FIND_CRITERION_NE, FIND_CRITERION_AND));
    }

    /* 带页数 */
    if (stPageInfo.nCurPage != -1)
    {
        MatchMethod &lastMethod = methods.back();
        lastMethod.enAndOr = FIND_CRITERION_NONE;
        
        /* 总个数, 要放在前面 */
        int nCount = -1;
        RecordFileDatabase::instance()->get_subDataCount(methods, nCount, DB_COMMON_FIELD_ID, strTargetTableName);
        stPageInfo.nDataTotal = nCount;
        /* 每页数据个数,默认20 */
        stPageInfo.nPageSize = stPageInfo.nPageSize == 0 ? 20 : stPageInfo.nPageSize;
        stPageInfo.nPageTotal = (stPageInfo.nDataTotal + stPageInfo.nPageSize - 1) / stPageInfo.nPageSize;

        /* 根据id升序 */
        std::string key = "order by " + std::string(DB_COMMON_FIELD_ID);
        methods.push_back(MatchMethod(Element(key, "asc"), FIND_CRITERION_NONE, FIND_CRITERION_NONE));
        /* 一页数据个数 */
        key = "limit";
        methods.push_back(MatchMethod(Element(key, stPageInfo.nPageSize), FIND_CRITERION_NONE, FIND_CRITERION_NONE));
        /* 第几页 */
        key = "OFFSET" ;
        methods.push_back(MatchMethod(Element(key, std::to_string(stPageInfo.nPageSize * (stPageInfo.nCurPage - 1))), FIND_CRITERION_NONE, FIND_CRITERION_NONE));
    }
    else
    {
        MatchMethod &lastMethod = methods.back();
        lastMethod.enAndOr = FIND_CRITERION_NONE;
        stPageInfo.nCurPage = 1;
        stPageInfo.nPageTotal = 1;
        int nCount = 0;
        RecordFileDatabase::instance()->get_subDataCount(methods, nCount, DB_COMMON_FIELD_ID, strTargetTableName);
        stPageInfo.nDataTotal = nCount;
    }

    if (methods.size() != 0)
    {
        MatchMethod &lastMethod = methods.back();
        lastMethod.enAndOr = FIND_CRITERION_NONE;
    }
 
    return RecordFileDatabase::instance()->find(methods, TsFileInfos, strTargetTableName);;
}

int RecordFileManage::retrieval(Record_NS::RetrievalCond_S stEventCond, std::vector<Record_NS::FileInfo_S> &infos)
{
    Common::PageInfo_S stPageInfo;
    return retrieval(stEventCond, infos, stPageInfo);
}

int RecordFileManage::retrieval(Record_NS::RetrievalCond_S stEventCond, std::vector<Record_NS::FileInfo_S> &infos, Common::PageInfo_S &stPageInfo)
{
    MatchMethods methods;
    std::lock_guard<std::mutex> lock(m_eventMutex);
    /* 多通道查询 */
    if (stEventCond.chnIds.size() > 0)
    {
        for (auto it = stEventCond.chnIds.begin(); it != stEventCond.chnIds.end(); ++it)
        {
            methods.push_back(MatchMethod(Element(RECORD_FILE_FIELD_CHN_ID, std::to_string(*it)), FIND_CRITERION_EQ, FIND_CRITERION_OR_P));
        }
        methods.back().enAndOr = FIND_CRITERION_AND;
    }
    if (stEventCond.nType != -1)
    {
        methods.push_back(MatchMethod(Element(RECORD_FILE_FIELD_TYPE, stEventCond.nType), FIND_CRITERION_EQ, FIND_CRITERION_AND));
    }
    /* 开始日期 */
    if (!stEventCond.strStartDate.empty())
    {
        std::string key= "strftime('%Y-%m-%d'," + std::string(RECORD_FILE_FIELD_CREATE_TIME) + ")";
        methods.push_back(MatchMethod(Element(key, stEventCond.strStartDate), FIND_CRITERION_GE, FIND_CRITERION_AND)); 
    }
    /* 结束日期 */
    if (!stEventCond.strEndDate.empty())
    {
        std::string key= "strftime('%Y-%m-%d'," + std::string(RECORD_FILE_FIELD_MODIFY_TIME) + ")";
        methods.push_back(MatchMethod(Element(key, stEventCond.strEndDate), FIND_CRITERION_IE, FIND_CRITERION_AND)); 
    }
    /* 查询加锁视频 */
    if (stEventCond.nIsLock != -1)
    {
        methods.push_back(MatchMethod(Element(RECORD_FILE_FIELD_IS_LOCK, stEventCond.nIsLock), FIND_CRITERION_EQ, FIND_CRITERION_AND)); 
    }
    /* 带页数 */
    if (stPageInfo.nCurPage != -1)
    {
        MatchMethod &lastMethod = methods.back();
        lastMethod.enAndOr = FIND_CRITERION_NONE;
        
        /* 总个数, 要放在前面 */
        int nCount = -1;
        RecordFileDatabase::instance()->get_count(methods, nCount, DB_COMMON_FIELD_ID);
        stPageInfo.nDataTotal = nCount;
        /* 每页数据个数,默认20 */
        stPageInfo.nPageSize = stPageInfo.nPageSize == 0 ? 20 : stPageInfo.nPageSize;
        stPageInfo.nPageTotal = (stPageInfo.nDataTotal + stPageInfo.nPageSize - 1) / stPageInfo.nPageSize;

        /* 根据id升序 */
        std::string key = "order by " + std::string(DB_COMMON_FIELD_ID);
        methods.push_back(MatchMethod(Element(key, "asc"), FIND_CRITERION_NONE, FIND_CRITERION_NONE));
        /* 一页数据个数 */
        key = "limit";
        methods.push_back(MatchMethod(Element(key, stPageInfo.nPageSize), FIND_CRITERION_NONE, FIND_CRITERION_NONE));
        /* 第几页 */
        key = "OFFSET" ;
        methods.push_back(MatchMethod(Element(key, std::to_string(stPageInfo.nPageSize * (stPageInfo.nCurPage - 1))), FIND_CRITERION_NONE, FIND_CRITERION_NONE));
    }
    else
    {
        MatchMethod &lastMethod = methods.back();
        lastMethod.enAndOr = FIND_CRITERION_NONE;
        stPageInfo.nCurPage = 1;
        stPageInfo.nPageTotal = 1;
        int nCount = 0;
        RecordFileDatabase::instance()->get_count(methods, nCount, DB_COMMON_FIELD_ID);
        stPageInfo.nDataTotal = nCount;
    }
    MatchMethod &lastMethod = methods.back();
    lastMethod.enAndOr = FIND_CRITERION_NONE;
    return RecordFileDatabase::instance()->find(methods, infos);
}

int RecordFileManage::get_itemInfo(Record_NS::FileInfo_S &stnfo)
{
    std::lock_guard<std::mutex> lock(m_eventMutex);
    std::vector<Record_NS::FileInfo_S> infos;
    RecordFileDatabase::instance()->find(Element(RECORD_FILE_FIELD_CHN_ID, stnfo.nChnId), infos);
    if (infos.size() == 0)
    {
        dlog_error("不存在");
        return -1;
    }
    stnfo = infos[0];
    return 0;
}

void RecordFileManage::deal_cacheFile()
{
    /* 处理缓存文件 */
    for (auto &map : m_tsFileInfosMap)
    {
        merge_video(map.second);
    }
    m_tsFileInfosMap.clear();
    std::lock_guard<std::mutex> lock(m_eventMutex);
    /* 结束事件 */
    for (auto it = m_eventFileMap.begin(); it != m_eventFileMap.end();)
    {
        if (it->second.m3u8 != nullptr)
        {
            it = m_eventFileMap.erase(it);
            continue;
        }
        it++;
    }
}

/**
 * @brief 处理ts文件
 * @param stTsFileInfo
 * @return * int
 */
int RecordFileManage::deal_tsFile(Record_NS::TsFileInfo_S stTsFileInfo)
{
    /* 取出对应通道的ts文件列表 */
    std::vector<Record_NS::TsFileInfo_S> &tsFileInfos = m_tsFileInfosMap[stTsFileInfo.nChnId];
    if (tsFileInfos.size() == 0)
    {
        if (stTsFileInfo.nIndex > 1)
        {
            dlog_warn("ts文件记录不连续");
        }
        tsFileInfos.push_back(stTsFileInfo);
        return 0;
    }

    /* 判断是否连续 */
    if (tsFileInfos.back().nIndex + 1 == stTsFileInfo.nIndex)
    {
        if (tsFileInfos.back().nSize * tsFileInfos.size() < 1024*1024) 
        {
            if(CRecordCtrl::instance()->is_newDay())
            {
                /*日期变更*/
                CRecordCtrl::instance()->update_recordDate();

                merge_video(tsFileInfos);

                tsFileInfos.clear();
                RecordFileDatabase::instance()->init_sub(TimeUtils_NS::get_currentDateAndFormat("%Y-%m-%d"));
                return 0;
            }
            /* 小于 1000M 记录并直接返回 */
            tsFileInfos.push_back(stTsFileInfo);
            return 0;
        }
        /* 连续约 1000M 分一次记录 */
        dlog_debug("主动分片视频");
    }
    /* 不连续或着自动分片，将记录组装成m3u8，再重新记录 */
    merge_video(tsFileInfos);
    
    /* 清除记录，重新记录 */
    tsFileInfos.clear();
    /* 添加记录 */
    tsFileInfos.push_back(stTsFileInfo);
    return 0;
}

void RecordFileManage::merge_video(std::vector<Record_NS::TsFileInfo_S> &tsFileInfos)
{
    if (tsFileInfos.size() == 0)
    {
        return;
    }
    /* 组装文件名 */
    std::string filename = tsFileInfos[0].createTime;
    /* 替换空格为下划线 */
    std::replace(filename.begin(), filename.end(), ' ', '_');
    /* 删除英文冒号 */
    filename.erase(std::remove(filename.begin(), filename.end(), ':'), filename.end());
    filename += ".m3u8";

    /* 填充文件信息 */
    Record_NS::FileInfo_S stFileInfo;
    stFileInfo.nChnId = tsFileInfos[0].nChnId;
    stFileInfo.path = tsFileInfos[0].path;
    stFileInfo.filename = filename;
    stFileInfo.createTime = tsFileInfos[0].createTime;

    /* 写入m3u8文件 */
    M3U8 m3u8(tsFileInfos[0].path + "/" + filename);
    for (auto &stInfo : tsFileInfos)
    {
        stFileInfo.nDuration += stInfo.nDuration;
        stFileInfo.nSize += stInfo.nSize;
        stFileInfo.modifyTime = tsFileInfos.back().modifyTime;
        M3U8::Data_S stData;
        stData.filename = stInfo.filename;
        stData.nDuration = stInfo.nDuration;
        stData.startTime = stInfo.createTime;
        m3u8.add_ts(stData);
    }
    stFileInfo.modifyTime = tsFileInfos.back().modifyTime;
    /* 文件类型:1分段文件 */
    stFileInfo.nType = 1;
    add(stFileInfo);
}

int RecordFileManage::deal_eventFile(Record_NS::TsFileInfo_S stTsFileInfo, int &nVideoType)
{
    int nRet = CEventLinkage::instance()->get_EventInfoMapSize();
    if(nRet != 0 || CRecordCtrl::instance()->get_RecordScheduleType() == 2)
    {
        nVideoType = 1; /* 事件类型视频 */
    }
    CEventLinkage::instance()->remove_EndedEvents(); 
    return 0;
}

#if 0
int RecordFileManage::deal_eventFile(Record_NS::TsFileInfo_S stTsFileInfo, int &nEventType)
{
    std::lock_guard<std::mutex> lock(m_eventMutex);

    /* 遍历事件 */
    for (auto it = m_eventFileMap.begin(); it != m_eventFileMap.end();)
    {
        auto &stEventType = it->first;
        auto &stEventFile = it->second;
        if (stEventType.nChnId != stTsFileInfo.nChnId)
        {
            it++;
            continue;
        }   
        /* 如果m3u8为空，说明是第一次记录 */
        if (stEventFile.m3u8 == nullptr)
        {
            stEventFile.stEventInfo.strStartTime = stTsFileInfo.createTime;
            /* 组装文件名 日期-类型-事件.m3u8 */
            std::string filename = stTsFileInfo.path + "/" + stEventFile.stEventInfo.strStartTime;
            filename += "_" + std::to_string(stEventType.nType) + "_";
            filename += ".m3u8";
            /* 替换空格为下划线 */
            std::replace(filename.begin(), filename.end(), ' ', '_');
            /* 删除英文冒号 */
            filename.erase(std::remove(filename.begin(), filename.end(), '-'), filename.end());
            filename.erase(std::remove(filename.begin(), filename.end(), ':'), filename.end());
            
            stEventFile.m3u8 = new M3U8(filename);
            stEventFile.stEventInfo.strVideoPath = filename;
            
            stEventFile.stEventInfo.strEndTime = stTsFileInfo.modifyTime;
            /* 更新事件 */
            EventDatabaseManage::instance()->update(stEventFile.stEventInfo);
            dlog_warn("更新事件[%d]",  stEventFile.stEventInfo.enType);
        }
        else
        {
            /* 序号是从1开始，如果进来stEventFile.m3u8不为空，说明创建的时候添加过ts文件，序号为1说明视频分段了 */
            if (stTsFileInfo.nIndex == 1)
            {
                /* TODO:如果事件时间为在此分片内，需要重新添加新事件 */
                dlog_warn("视频分段，事件结束");
                
                Log::Info_S stLogInfo;
                stLogInfo.startTime = stEventFile.stEventInfo.strStartTime;
                stLogInfo.nType = Log::Type::ALARM;
                stLogInfo.nAction = Log::to_action(stEventFile.stEventInfo.enType, false /* isStart */);
                stLogInfo.chnName = std::string("D") + std::to_string(stEventType.nChnId);
                stLogInfo.context = "人员";
                LogHandler::instance()->write(stLogInfo);  

                it = m_eventFileMap.erase(it);
                continue;
            }   
            stEventFile.stEventInfo.strEndTime = stTsFileInfo.modifyTime;
            /* 更新事件 */
            EventDatabaseManage::instance()->update(stEventFile.stEventInfo);
            dlog_warn("更新事件");
        }
        /* 添加ts文件 */
        M3U8::Data_S stData;
        stData.filename = stTsFileInfo.filename;
        stData.nDuration = stTsFileInfo.nDuration;
        stData.startTime = stTsFileInfo.createTime;
        stEventFile.m3u8->add_ts(stData);
        /* 记录事件类型更新到记录ts文件的数据库中 */
        nEventType = (int)stEventFile.stEventInfo.enType;

        stEventFile.stEventInfo.nVideoSize += stTsFileInfo.nSize;
        /* 判断事件是否结束 */
        std::string eventTime = stEventFile.stEventInfo.strDate + " " + stEventFile.stEventInfo.strTime;
        if (stTsFileInfo.createTime > eventTime)
        {
            if (stEventFile.m3u8)
            {
                delete stEventFile.m3u8;
                stEventFile.m3u8 = nullptr;
            }
            
            Log::Info_S stLogInfo;
            stLogInfo.startTime = stEventFile.stEventInfo.strStartTime;
            stLogInfo.nType = Log::Type::ALARM;
            stLogInfo.nAction = Log::to_action(Event::Type(stEventType.nType), false /* isStart */);
            stLogInfo.chnName = std::string("D") + std::to_string(stEventType.nChnId);
            stLogInfo.context = "人员";
            LogHandler::instance()->write(stLogInfo);
            it = m_eventFileMap.erase(it);
            dlog_warn("事件结束");
        }
        else
        {
            it++;
        }
    }

    return 0;
}
#endif

/* 根据ts文件名转化为录制的日期(数据库中的表名) */
std::string tsFilenameconvertDate(const std::string& strTSFilename) 
{
    /* 20250826_101730.ts 转换为 2025-08-26 */
    if (strTSFilename.length() == 0)
    {
        return std::string();
    }
    
    std::string datePart = strTSFilename.substr(0, 8);
    
    try 
    {
        int year = stoi(datePart.substr(0, 4));
        int month = stoi(datePart.substr(4, 2));
        int day = stoi(datePart.substr(6, 2));
        
        // 创建一个日期对象
        tm date = {};
        date.tm_year = year - 1900; // tm_year是从1900开始的年数
        date.tm_mon = month - 1;    // tm_mon是0-11
        date.tm_mday = day;
        
        // 格式化输出
        char buffer[11];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d", &date);
        return std::string(buffer);
    } 
    catch (...) 
    {
        return std::string();
    }
}

#if 0
int RecordFileManage::create_eventVideo(Event::Info_S &stEventInfo)
{
    /* 事件类型 */
    EventType_S stEventType;
    stEventType.nChnId = stEventInfo.nChnId;
    stEventType.nType = int(stEventInfo.enType);
    stEventType.nUniqueId = stEventInfo.nId;

    std::lock_guard<std::mutex> lock(m_eventMutex);
    /* 拿出对应通道的ts文件 */
    std::vector<Record_NS::TsFileInfo_S> &tsFileInfos = m_tsFileInfosMap[stEventInfo.nChnId];
    if (tsFileInfos.size() == 0)
    {
        dlog_warn("事件[%d-%d]无ts文件", stEventInfo.nChnId, stEventInfo.enType);
        
        Log::Info_S stLogInfo;
        stLogInfo.startTime = stEventInfo.strStartTime;
        stLogInfo.nType = Log::Type::ALARM;
        stLogInfo.nAction = Log::to_action(Event::Type(stEventType.nType), false /* isStart */);
        stLogInfo.chnName = std::string("D") + std::to_string(stEventType.nChnId);
        stLogInfo.context = "人员";
        LogHandler::instance()->write(stLogInfo);
        return -1;
    }
    /* 拿出对应事件文件 */
    auto &stEventFile = m_eventFileMap[stEventType];
    /* 填充/更新事件信息 */
    if (stEventFile.stEventInfo.nChnId == -1)
    {
        stEventFile.stEventInfo.nId = stEventInfo.nId;
        stEventFile.stEventInfo.enType = stEventInfo.enType;
        stEventFile.stEventInfo.nChnId = stEventInfo.nChnId;
        stEventFile.stEventInfo.strDate = stEventInfo.strDate;
        stEventFile.stEventInfo.strTime = stEventInfo.strTime;
        stEventFile.stEventInfo.nVideoBindId = stEventInfo.nVideoBindId;
    }
    else
    {
        stEventFile.stEventInfo.strDate = stEventInfo.strDate;
        stEventFile.stEventInfo.strTime = stEventInfo.strTime;
    }

    if (stEventFile.m3u8 == nullptr)
    {
        stEventFile.stEventInfo.strStartTime = tsFileInfos.back().createTime;
        /* 组装文件名 日期-类型-事件.m3u8 */
        std::string filename = tsFileInfos.back().path + "/" + stEventFile.stEventInfo.strStartTime;
        filename += "_" + std::to_string(int(stEventInfo.enType)) + "_";
        filename += ".m3u8";
        /* 替换空格为下划线 */
        std::replace(filename.begin(), filename.end(), ' ', '_');
        /* 删除英文冒号 */
        filename.erase(std::remove(filename.begin(), filename.end(), '-'), filename.end());
        filename.erase(std::remove(filename.begin(), filename.end(), ':'), filename.end());
        stEventFile.m3u8 = new M3U8(filename);
        stEventInfo.strVideoPath = filename;
        /* 添加ts文件 */
        M3U8::Data_S stData;

        stData.filename = tsFileInfos.back().filename;
        stData.nDuration = tsFileInfos.back().nDuration;
        stData.startTime = tsFileInfos.back().createTime;
        stEventFile.m3u8->add_ts(stData);

        /* 更新ts录像文件信息 */
        std::string strTargetTableName = tsFilenameconvertDate(tsFileInfos.back().filename);
        if(!strTargetTableName.empty())
        {
            tsFileInfos.back().nType = (int)stEventFile.stEventInfo.enType;
            update(tsFileInfos.back(), strTargetTableName); 
        }

        stEventFile.stEventInfo.nVideoSize += tsFileInfos.back().nSize;
        stEventFile.stEventInfo.strVideoPath  = filename;
        stEventFile.stEventInfo.strEndTime = tsFileInfos.back().modifyTime;
        /* 更新事件 */
        EventDatabaseManage::instance()->update(stEventFile.stEventInfo);
        dlog_warn("更新事件[%d]",  stEventFile.stEventInfo.enType);

    }
    else
    {
        stEventInfo.strVideoPath = stEventFile.stEventInfo.strVideoPath;
        dlog_info("事件[%d-%d]已存在", stEventFile.stEventInfo.nChnId, stEventFile.stEventInfo.enType);
    }
    return 0;
}
#endif

static long getFileSize(const std::string& filename) 
{
    struct stat fileStat;
    if (stat(filename.c_str(), &fileStat) != 0) 
    {
        dlog_error("get ts file size error");
        return -1;
    }
    return fileStat.st_size;
}

int RecordFileManage::add_eventVideo(std::string &strM3u8Path, std::string &strM3u8FileName, Event::Info_S &stEventInfo)
{
    M3U8 m3u8;
    long lTsTotalSize = 0; 
    long nSize = 0;
    std::string strFullM3u8Path = strM3u8Path + "/" + strM3u8FileName;
    std::lock_guard<std::mutex> lock(m_eventMutex);

    /* 解析M3U8文件并提取.ts文件名 */
    std::vector<std::string> strTsFiles = m3u8.get_M3u8TsFileName(strFullM3u8Path);

    /* 计算m3u8文件中ts文件的总大小 */
    for (const auto& strTsFile : strTsFiles) 
    {
        std::string strFullTsPath = strM3u8Path + "/" + strTsFile;
        nSize = getFileSize(strFullTsPath);
        if(nSize > 0)
        {
            lTsTotalSize += nSize;
        }
    }

    stEventInfo.nVideoSize = lTsTotalSize;
    stEventInfo.strVideoPath = strFullM3u8Path;
    stEventInfo.strEndTime = TimeUtils_NS::get_currentTimeAndFormat("%H:%M:%S");

    /* 更新事件 */
    EventDatabaseManage::instance()->update(stEventInfo);
    return 0;
}

int RecordFileManage::create_temporaryVideo(const std::string filename, int nChnId)
{
    std::lock_guard<std::mutex> lock(m_eventMutex);
    /* 拿出对应通道的ts文件 */
    if (m_tsFileInfosMap.find(nChnId) == m_tsFileInfosMap.end())
    {
        return -1; 
    }
    std::vector<Record_NS::TsFileInfo_S> &tsFileInfos = m_tsFileInfosMap[nChnId];
    if (tsFileInfos.size() == 0)
    {
        return -1;
    }
    std::remove(filename.c_str());
    auto m3u8 = new M3U8(filename);
    /* 添加ts文件, 最多50个 */
    auto it = tsFileInfos.size() > 50 ? tsFileInfos.end() - 50 : tsFileInfos.begin();
    for (; it != tsFileInfos.end(); it++)
    {
        M3U8::Data_S stData;
        stData.filename = it->path + "/" + it->filename;
        stData.nDuration = it->nDuration;
        stData.startTime = it->createTime;
        m3u8->add_ts(stData);
    }
    return 0;
}


double  RecordFileManage::get_channel_size(std::string strPath) 
{
    if(strPath.empty())
    {
        return 0.00;
    }

    //dlog_info("获取通道路径：%s的已用容量",strPath.c_str());
    std::string strCommand = "du -sh " + strPath + " 2>/dev/null"; 
    std::array<char, 128> buffer;
    std::string strResult;

    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(strCommand.c_str(), "r"), pclose);
    if (!pipe) 
    {
        dlog_error("pipe失败");
        return 0.00;
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) 
    {
        strResult += buffer.data();
    }
    //dlog_info("du -sh获取到的数据：%s",strResult.c_str());

    std::regex regex(R"((\d+(\.\d+)?)([KMGTP]))");  
    std::smatch match;
    std::string strSizeStr;

    if (std::regex_search(strResult, match, regex))
    {
        strSizeStr = match[1].str() + match[3].str();
        dlog_info("du -sh提取数值部分：%s",strSizeStr.c_str());
    }
    else
    {
        return 0.00;
    }

    /* 获取单位 */
    char unit = std::toupper(strSizeStr.back());
    /* 提取数值部分 */
    double dSizeValue = std::stod(strSizeStr.substr(0, strSizeStr.size() - 1));
    dlog_info("数值部分提取：%s 单位：%c",std::to_string(dSizeValue).c_str(),unit);
    switch (unit) 
    {
        case 'K': 
            return std::round(dSizeValue / (1024 * 1024) * 100.0) / 100.0;   // KB 转 GB
        case 'M': 
             return std::round(dSizeValue / 1024 * 100.0) / 100.0;           // MB 转 GB     
        case 'G': 
            return std::round(dSizeValue * 100.0) / 100.0;                   // GB
        case 'T': 
            return std::round(dSizeValue * 1024 * 100.0) / 100.0;            // TB 转 GB
        default : return 0.00;
    }

}

/* 去除字符串两端空白 */
static std::string trim(const std::string &str)
{
    auto start = str.begin();
    while (start != str.end() && std::isspace(*start))
    {
        start++;
    }

    auto end = str.end();
    do
    {
        end--;
    } while (std::distance(start, end) > 0 && std::isspace(*end));

    return std::string(start, end + 1);
}

std::vector<std::string> RecordFileManage::findM3u8Dates(const std::string &strPath, const std::string &strPrefix)
{
    std::vector<std::string> vecResult;
    std::error_code          ec;

    fs::directory_iterator it(strPath, ec);
    if (ec)
    {
        dlog_error("opendir %s", strPath.c_str());
        return vecResult;
    }

    for (const auto &entry : it)
    {
        if (!entry.is_directory())
        {
            continue;
        }
            
        const auto strFilename = entry.path().filename().string();
        if (strFilename.size() != 8)
        {
            continue;
        }
            
        if (!std::all_of(strFilename.begin(), strFilename.end(), [](unsigned char c) { return std::isdigit(c); }))
        {
            continue;
        }
            
        auto m3u8 = entry.path() / (strPrefix + "_" + strFilename + ".m3u8");

        if (!fs::exists(m3u8))
        {
            continue;
        }
            
        vecResult.emplace_back(strFilename.substr(0, 4) + "-" + strFilename.substr(4, 2) + "-" + strFilename.substr(6, 2));
    }

    std::sort(vecResult.begin(), vecResult.end());
    return vecResult;
}


/* 删除ts文件时同步修改对应的m3u8文件 */
static int deal_oldest_segments(const std::string inputFile)
{
    const std::string tempFile = "playlist_temp.m3u8";

    // 打开输入文件和临时输出文件
    std::ifstream infile(inputFile);
    if (!infile.is_open())
    {
        dlog_error("Error: Could not open input file ");
        return 1;
    }

    std::ofstream outfile(tempFile);
    if (!outfile.is_open())
    {
        dlog_error("Error: Could not create temporary file ");
        infile.close();
        return 1;
    }

    // 状态跟踪变量
    bool bMediaSeqUpdated = false;
    bool bFragmentDeleted = false;
    int nMediaSeqValue = 0;
    int nSkipLines = 0;

    std::string line;
    while (std::getline(infile, line))
    {
        std::string trimmed = trim(line);

        // 跳过需要删除的行
        if (nSkipLines > 0)
        {
            nSkipLines--;
            continue;
        }

        // 处理媒体序列号
        if (!bMediaSeqUpdated && trimmed.find("#EXT-X-MEDIA-SEQUENCE:") == 0)
        {
            std::string valueStr = trimmed.substr(22);
            try
            {
                nMediaSeqValue = std::stoi(trim(valueStr));
                outfile << "#EXT-X-MEDIA-SEQUENCE:" << (nMediaSeqValue + 1) << "\n";
                bMediaSeqUpdated = true;
                continue;
            }
            catch (...)
            {
                // 解析失败则保留原行
                outfile << line << "\n";
            }
        }
        // 处理第一个片段
        else if (!bFragmentDeleted && trimmed.find("#EXT-X-PROGRAM-DATE-TIME:") == 0)
        {
            // 跳过接下来的两行（EXTINF和TS文件名）
            nSkipLines = 2;
            bFragmentDeleted = true;

            // 确保媒体序列号已更新
            if (!bMediaSeqUpdated)
            {
                dlog_warn("Deleting fragment before updating media sequence, Adding default media sequence.");
                outfile << "#EXT-X-MEDIA-SEQUENCE:1\n";
                bMediaSeqUpdated = true;
            }
        }
        // 普通行处理
        else
        {
            outfile << line << "\n";
        }
    }

    // 关闭文件流
    infile.close();
    outfile.close();

    // 检查操作结果
    if (!bFragmentDeleted)
    {
        dlog_error("Error: No fragments found to delete.");
        std::remove(tempFile.c_str());
        return 1;
    }

    if (!bMediaSeqUpdated)
    {
        dlog_error("Error: Media sequence not found and no fragments deleted.");
        std::remove(tempFile.c_str());
        return 1;
    }

    // 替换原文件
    if (std::rename(tempFile.c_str(), inputFile.c_str()))
    {
        dlog_error("Error: Failed to replace original file.");
        return 1;
    }

    // dlog_info("Successfully removed oldest fragment.");

    return 0;
}

/* 从录制目录中过滤出日期最旧的一个 */
static std::optional<std::string> getOldestDateDir(const fs::path &root, std::string *err = nullptr)
{
    try
    {
        std::optional<std::string> minDir;
        for (const auto &entry : fs::directory_iterator(root))
        {
            if (!entry.is_directory())
            {
                continue;
            }

            const std::string name = entry.path().filename().string();
            if (name.length() != 8 || !std::all_of(name.begin(), name.end(), ::isdigit))
            {
                continue;
            }

            if (!minDir || name < *minDir)
            {
                minDir = name;
            }
        }
        return minDir;
    } catch (const fs::filesystem_error &ex)
    {
        if (err)
        {
            *err = ex.what();
        }
        return std::nullopt;
    }
}

/* 获取一个较小的日期 */
static std::string compare_date(const std::string& strDate1, const std::string& strDate2)
{
    auto to_time_t = [](const std::string& s) -> std::time_t
    {
        std::tm tm = {};
        tm.tm_year = std::stoi(s.substr(0, 4)) - 1900;
        tm.tm_mon  = std::stoi(s.substr(4, 2)) - 1;
        tm.tm_mday = std::stoi(s.substr(6, 2));
        tm.tm_hour = tm.tm_min = tm.tm_sec = 0;
        tm.tm_isdst = -1;
        return std::mktime(&tm);
    };

    std::time_t t1 = to_time_t(strDate1);
    std::time_t t2 = to_time_t(strDate2);

    std::string strResult;
    if (t1 < t2)
    {
        strResult = strDate1;
    }
    else
    {
        strResult = strDate2;
    }
    return strResult;
}

int RecordFileManage::loop_write()
{
    std::vector<std::string> strTables;
    std::string strTable;
    std::string strRecordDateDirName;
    std::string strRecordDbName;
    int nCount = 0;
    int nRet;

    std::lock_guard<std::mutex> lock(m_eventMutex);

    fs::path  strPath = RECORD_PATH;
    std::string err;

    /* 从录制目录中过滤出日期最旧的一个目录 */
    auto oldestDir = getOldestDateDir(strPath, &err);
    
    if (oldestDir)
    {
        strRecordDateDirName = *oldestDir;
    }
    else
    {
        std::cerr << "failed: "  << (err.empty() ? "no valid dir" : err) << '\n';
        return -1;
    }

    /* 获取当天日期 */
    std::time_t now = std::time(nullptr);
    std::tm local_tm{};
    localtime_r(&now, &local_tm);
    std::ostringstream oss;
    oss << std::put_time(&local_tm, "%Y-%m-%d");
    std::string strCurrentDate = oss.str();

    strRecordDbName = strRecordDateDirName;
    strRecordDbName.insert(4, "-").insert(7, "-");   // 20251010 -> 2025-10-10
  
    /* 比较最旧的录制目录是否是当天的录制目录 */
    if(strRecordDbName == strCurrentDate)
    {
        CRecordCtrl::instance()->stop_record();
        dlog_info("停止录制");
        /* 等待停止录制 */
        sleep(1);
    }

    /* 获取数据库所有表名 */
    strTables = RecordFileDatabase::instance()->get_all_tables();
    if(strTables.size() <= 2)
    {
        dlog_error("没有查询到录制文件表格")
        // return -1;
    }

    strTables.erase(std::remove(strTables.begin(), strTables.end(), RECORD_FILE_TABLE_NAME), strTables.end());
    strTables.erase(std::remove(strTables.begin(), strTables.end(), RECORD_DIR_INFO_TABLE_NAME), strTables.end());
    
    std::vector<std::string> strDeleteTables;
    for (const auto& table : strTables) 
    {
        std::string strCleaned = table;

        strCleaned.erase(std::remove(strCleaned.begin(), strCleaned.end(), '-'), strCleaned.end());

        if (compare_date(strRecordDateDirName, strCleaned) == strCleaned) 
        {
            strDeleteTables.push_back(table);
        }
    }

    /* 删除录制文件数据库中符合条件的录制文件表格 */ 
    for (const auto& table : strDeleteTables) 
    {
        nCount += RecordFileDatabase::instance()->get_table_data_count(table);
        if(strCurrentDate != table)
        {
            RecordFileDatabase::instance()->del_table(table);
        }
        else 
        {
            RecordFileDatabase::instance()->clear_table(table);
        }
    }

    /* 删除录制文件数据库管理表格中符合条件的数据 */  
    MatchMethods recordMethods;
    Event::RetrievalCond_S stRecordCond;
    stRecordCond.strTime = strRecordDbName + " " + "23:59:59";
    recordMethods.push_back(MatchMethod(Element(RECORD_FILE_FIELD_CREATE_TIME, stRecordCond.strTime), FIND_CRITERION_IE, FIND_CRITERION_AND));
    if (recordMethods.size() != 0)
    {
        MatchMethod &lastMethod = recordMethods.back();
        lastMethod.enAndOr = FIND_CRITERION_NONE;
    }
    RecordFileDatabase::instance()->del(recordMethods, RECORD_FILE_TABLE_NAME);

    /* 删除事件数据库中符合条件的数据 */
    MatchMethods eventMethods;
    Event::RetrievalCond_S stEventCond;
    stEventCond.strStartDate = strRecordDbName;
    eventMethods.push_back(MatchMethod(Element(Event::INFO_EVENT_DATE, stEventCond.strStartDate), FIND_CRITERION_IE, FIND_CRITERION_AND));
    if (eventMethods.size() != 0)
    {
        MatchMethod &lastMethod = eventMethods.back();
        lastMethod.enAndOr = FIND_CRITERION_NONE;
    }
    EventDatabase::instance()->del(eventMethods, EVENT_TABLE_NAME);

    /* 删除录制文件 */
    std::string strSourcePath = std::string(RECORD_PATH) + "/" + strRecordDateDirName;
    std::string strTrashDirPath = std::string(RECORD_PATH) + "/.trash";
    std::string strTmpDirPath = strTrashDirPath + "/" + strRecordDateDirName;
    char strBuf[128] = {0};
    long long llDirSize = 0;

    /* 获取要删除的目录的大小 */
    std::string strDuCmd = "du -sb \"" + strSourcePath + "\"";
    FILE* pipe = popen(strDuCmd.c_str(), "r");
    if (pipe)
    {
        if (fgets(strBuf, sizeof(strBuf), pipe) != nullptr) 
        {
            llDirSize = std::atoll(strBuf);
        }        
        pclose(pipe);
    }

    /* 确保 .trash 存在 */
    std::string strMkdirCmd = "mkdir -p \"" + strTrashDirPath + "\"";
    std::system(strMkdirCmd.c_str());

    /* 移动目录 */
    std::string strMvCmd = "mv \"" + strSourcePath + "\" \"" + strTmpDirPath + "\"";
    int mvStatus = std::system(strMvCmd.c_str());
    if (WIFEXITED(mvStatus) && WEXITSTATUS(mvStatus) == 0) 
    {
        dlog_info("Moved to trash: [%s]", strMvCmd.c_str());

        /* 删除旧录制目录 */
        std::string strRmCmd = "ionice -c3 rm -rf \"" + strTmpDirPath + "\"";
        nRet = std::system(strRmCmd.c_str());
        if (nRet == 0) 
        {
            dlog_info("deleted success [%s]", strRmCmd.c_str());
        }
        else 
        {
            dlog_error("failed to delete %s, code %d", strRmCmd.c_str(), nRet);
        }

        /* 更新记录的录制目录大小 */
        Record_NS::RecordDirInfo_S stDirInfo;
        stDirInfo.nChnId = 0;
        nRet = RecordFileDatabase::instance()->get_itemInfo(stDirInfo);

        stDirInfo.nTotalSize -= llDirSize;
        stDirInfo.nCount -= nCount;

        RecordFileDatabase::instance()->update(stDirInfo);
    } 
    else 
    {
        dlog_error("Failed to move to trash: [%s]", strMvCmd.c_str());
        nRet = -1;
    }

    /* 比较最旧的录制目录是否是当天的录制目录 */
    if(strRecordDbName == strCurrentDate)
    {
        /* 恢复录制 */
        CRecordCtrl::instance()->start_record();
        dlog_info("恢复录制");
    }
    
    return nRet;
}

int RecordFileManage::rm_recordDir(const fs::path strRecordBasePath, time_t nTime)
{
    dlog_info("同步删除相关录制目录");
    char strTimeDateBuf[32] = {0};
    std::tm tmBuf{};

    localtime_r(&nTime, &tmBuf);
    std::strftime(strTimeDateBuf, sizeof(strTimeDateBuf), "%Y%m%d_%H%M%S", &tmBuf);
    
    /* 得到 20250923_104053格式时间字符串 */
    std::string baseDateTime = strTimeDateBuf;

    /* 提取出20250923目录名字 */
    std::string strDate = baseDateTime.substr(0, baseDateTime.find('_'));

    if (strDate.size() != 8 || strDate.find_first_not_of("0123456789") != std::string::npos) 
    {
        dlog_error("无效的日期格式");
        return -1;
    }

    try 
    {
        #if 0
        for (const auto& entry : fs::directory_iterator(strRecordBasePath)) 
        {
            if (!entry.is_directory())
            {
                continue;
            } 
            const std::string name = entry.path().filename().string();
            if (name.size() != 8)
            {
                continue;
            }
            if (name <= strDate)
            {
                /* 只会删除 strDate > name */
                continue;
            }
             
            if (name.find_first_not_of("0123456789") != std::string::npos) 
            {
                continue;
            }
            
            dlog_info("removing %s", entry.path().c_str());
            fs::remove_all(entry.path());
        }
        #endif

        fs::path trash = fs::path(strRecordBasePath) / ".trash";
        fs::create_directories(trash);
        /* 记录移动过去的目录 */ 
        std::vector<fs::path> movedDirs;          

        for (const auto& entry : fs::directory_iterator(strRecordBasePath))
        {
            if (!entry.is_directory())
            {
                continue;
            } 

            const std::string name = entry.path().filename().string();
            if (name.size() != 8)
            {
                continue;
            } 
            if (name <= strDate) 
            {
                /* 只会删除 strDate > name */
                continue;
            }
            if (name.find_first_not_of("0123456789") != std::string::npos) continue;

            fs::path dst = trash / name;
            try
            {
                fs::rename(entry.path(), dst);
                movedDirs.push_back(dst);
                dlog_info("moved %s -> trash", entry.path().c_str());
            }
            catch (const fs::filesystem_error& ex)
            {
                dlog_error("move %s failed: %s", entry.path().c_str(), ex.what());
            }
        }

        /* 后台慢删（单线程+低优先级） */
        if (!movedDirs.empty())
        {
            std::thread([trash]{ 
                ::nice(19);  /* CPU 最低 */
                for (fs::directory_iterator it(trash), end; it != end; ++it) 
                {
                    if (!it->is_directory())
                    {
                        continue;
                    }
                    
                    std::string cmd = "ionice -c3 rm -rf " + it->path().string();
                    
                    int rc = std::system(cmd.c_str());
                    
                    if (rc == 0)
                    {
                        dlog_info("deleted %s", it->path().c_str());
                    }
                    else
                    {
                        dlog_error("failed to delete %s, code %d", it->path().c_str(), rc);
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }
            }).detach();
        }

    } 
    catch (const fs::filesystem_error& e) 
    {
        dlog_error("rm_recordDir error:%s", e.what());
        return -1;
    }

    return 0;
}

int RecordFileManage::rm_recordTsFile(const fs::path strRecordBasePath, time_t nTime) 
{
    dlog_info("同步删除相关ts录制文件");
    char strTimeDateBuf[32] = {0};
    std::tm tmBuf{};

    localtime_r(&nTime, &tmBuf);
    std::strftime(strTimeDateBuf, sizeof(strTimeDateBuf), "%Y%m%d_%H%M%S", &tmBuf);
    
    /* 得到 20250923_104053格式时间字符串 */
    std::string baseDateTime = strTimeDateBuf;

    /* 提取出20250923目录名字 */
    std::string strDate = baseDateTime.substr(0, baseDateTime.find('_'));
    /* 拼接出 /opt/course/record/20250923 录制路径*/
    std::string strRecordFullPath = strRecordBasePath.string() + "/" + strDate;

    if(!std::filesystem::exists(strRecordFullPath) || !std::filesystem::is_directory(strRecordFullPath))
    {
        dlog_info("目录%s不存在", strRecordFullPath.c_str());
        return 0;
    }

    try 
    {
        uint64_t totalSize  = 0;   // 目录总大小
        uint64_t deleteSize = 0;   // 已删大小
        uint64_t leftSize   = 0;   // 剩余大小
        int deleteCount = 0;
        int totalCount = 0;
        // std::vector<fs::path> batch;
        // batch.reserve(100);
        
        dlog_debug("开始清理目录：%s", strRecordFullPath.c_str());
        dlog_debug("从时间[%s]开始删除", baseDateTime.c_str());

        // time_t start_time = time(NULL);       

        for (const auto& entry : fs::directory_iterator(strRecordFullPath)) 
        {
            if (!entry.is_regular_file()) continue;
            
            std::string filename = entry.path().filename().string();
            uint64_t  fileSize   = entry.file_size();
            totalSize += fileSize;
            totalCount++;

            /* 检查是否是.ts文件且符合命名格式 */
            if (filename.length() == 18 && filename.substr(filename.length() - 3) == ".ts") 
            {
                /* 提取日期时间部分（去掉.ts扩展名） */
                std::string fileDateTime = filename.substr(0, 15);
                
                /* 比较日期时间 */ 
                if (fileDateTime >= baseDateTime) 
                {
                    fs::remove(entry.path());
                    // usleep(1000);
                    deleteCount++;
                    deleteSize += fileSize;     /* 累加删除大小 */

                    // batch.push_back(entry.path());
                    
                }
                usleep(1000);
            }

            // if (batch.size() == 100) 
            // {
            //     std::string cmd = "rm -f";
            //     for (auto& p : batch)
            //     {
            //         cmd += " " + p.string();
            //     }
            //     /* 一次性删 100 个 */ 
            //     std::system(cmd.c_str());  
            //     batch.clear();
            //     dlog_debug("cmd:%s", cmd.c_str());
            //     /* sleep 10ms 避免cpu占用过高 */
            //     usleep(10 * 1000);
            // }
        }
        // /* 把剩下的进行删除 */
        // if(batch.size() > 0)
        // {
        //     std::string cmd = "rm -f";
        //     for (auto& p : batch) 
        //     {
        //         cmd += " " + p.string();
        //     }
        //     std::system(cmd.c_str());
        //     dlog_debug("cmd:%s", cmd.c_str());
        // }
        leftSize = totalSize - deleteSize;      // 得到剩余大小

        // dlog_debug(" ============= 删除完成,耗时 %lld s ============= ", time(NULL) - start_time);
        dlog_info("删除相关录制文件完成，删除了%d/%d个文件", deleteCount, totalCount);
        
        // /* 把删除的的ts文件的大小更新到记录目录信息的文件里面 */
        Record_NS::RecordDirInfo_S stRecordDirInfo;
        long long llSize = 0;
        stRecordDirInfo.nChnId = 0;
        CStorageManage::instance()->get_directory_size(llSize, RECORD_PATH);
        int nRet = RecordFileDatabase::instance()->get_itemInfo(stRecordDirInfo);
        stRecordDirInfo.nTotalSize = llSize;
        if(nRet < 0)
        {
            RecordFileDatabase::instance()->add(stRecordDirInfo);
        }
        else 
        {
            RecordFileDatabase::instance()->update(stRecordDirInfo);
        }

        /* 只要找到一个 .ts 文件就立即返回 */
        fs::path dir{strRecordFullPath};
        bool bExitTsFile = false;
        for (const auto& e : fs::directory_iterator(dir))
        {
            if (e.is_regular_file() && e.path().extension() == ".ts")
            {   
                bExitTsFile = true;
                break;
            }   
        }
        /* 如果这个目录不存在ts文件了，则进行删除 */
        if(!bExitTsFile)
        {
            /* 没有 .ts 文件：先切到父目录再删，避免占用当前工作目录 */
            fs::path parent = dir.parent_path();
            if (!parent.empty()) 
            {
                fs::current_path(parent);
            }
            
            /* 递归删除（目录已空） */ 
            fs::remove_all(dir); 
            dlog_debug("当前目录%s没有ts文件了，进行删除", strRecordFullPath.c_str()); 
        }
    } 
    catch (const fs::filesystem_error& e) 
    {
        dlog_error("rm_RecordTsFile error:%s", e.what());
        return -1;
    }
    
    return 0;
}

int RecordFileManage::del_recordfilemanageDbInfo(time_t nTime)
{
    dlog_info("同步录制文件表格");
    std::string strTargetTable;
    std::vector<std::string> strTables;

    Event::RetrievalCond_S stCond;

    char strTimeDateBuf[32] = {0};
    std::tm tmBuf{};

    localtime_r(&nTime, &tmBuf);
    std::strftime(strTimeDateBuf, sizeof(strTimeDateBuf), "%Y-%m-%d %H:%M:%S", &tmBuf);

    std::string datetime = strTimeDateBuf;
    size_t spacePos = datetime.find(' ');
    if (spacePos == std::string::npos) 
    {
        return -1;
    }
    /* 从2025-09-06 15:28:58中提取2025-09-06形式的表名 */
    strTargetTable = datetime.substr(0, spacePos);

    /* 获取数据库所有表名 */
    strTables = RecordFileDatabase::instance()->get_all_tables();
    if(strTables.size() < 2)
    {
        dlog_error("没有查询到录制文件表格")
        return -1;
    }
    /* 排序（升序） */
    std::sort(strTables.begin(), strTables.end());

    /* 遍历并比较 */
    for(unsigned int i = 0; i < strTables.size(); i++)
    {
        const auto& table = strTables.at(i);
        if ( (table > strTargetTable) && (table != RECORD_FILE_TABLE_NAME) && (table != RECORD_DIR_INFO_TABLE_NAME) )
        {
            // std::cout << "表名比 " << strTargetTable << " 大: " << table << std::endl;
            RecordFileDatabase::instance()->del_table(table);
        } 
    }

    if (spacePos != std::string::npos && spacePos + 1 < datetime.length()) 
    {
        /* 返回空格后的所有内容 */
        stCond.strTime = datetime.substr(spacePos + 1);
    }

    if(!stCond.strTime.empty())
    {
        RecordFileManage::instance()->del(stCond, strTargetTable);
    }

    return 0;
}

int RecordFileManage::del_eventmanageDbInfo(time_t nTime)
{
    dlog_info("同步删除相关eventmanage表格");
    Event::RetrievalCond_S stCond;
    char strTimeDateBuf[32] = {0};
    std::tm tmBuf{};

    localtime_r(&nTime, &tmBuf);
    std::strftime(strTimeDateBuf, sizeof(strTimeDateBuf), "%Y-%m-%d %H:%M:%S", &tmBuf);

    // stCond.strEndTime = "2025-09-06 15:28:58";
    stCond.strEndTime = strTimeDateBuf;
    EventDatabaseManage::instance()->del(stCond);
    return 0;
}

static std::time_t fastParseTime(const std::string& line) 
{
    size_t lastColon = line.rfind(':');
    if (lastColon == std::string::npos || lastColon < 19) 
    {
        return 0;
    }

    // 尝试定位到日期开始处 (YYYY-MM-DD)，格式固定，截取最后 19 位
    const char* p = line.c_str() + line.length() - 19;
    
    // 基本校验：检查是否为数字
    if (!isdigit(p[0])) 
    {
        // 如果最后 19 位不是时间，尝试寻找空格后的内容
        size_t spacePos = line.rfind(' ');
        if (spacePos != std::string::npos && line.length() - spacePos >= 9) 
        {
            p = line.c_str() + spacePos - 10; // 指向日期开始
        } 
        else 
        {
            return 0;
        }
    }

    struct tm tm = {0};
    if (sscanf(p, "%4d-%2d-%2d%*c%2d:%2d:%2d", 
               &tm.tm_year, &tm.tm_mon, &tm.tm_mday, 
               &tm.tm_hour, &tm.tm_min, &tm.tm_sec) == 6) 
    {
        tm.tm_year -= 1900;
        tm.tm_mon -= 1;
        tm.tm_isdst = -1;
        // 使用 UTC时间，本地时间则用 mktime
        return timegm(&tm); 
    }
    return 0;
}

int RecordFileManage::truncateM3U8(time_t nTime) 
{
    dlog_info("同步m3u8文件")
    fs::path filePath; 

    char strTimeDateBuf[32] = {0};
    std::tm tmBuf{};

    localtime_r(&nTime, &tmBuf);
    std::strftime(strTimeDateBuf, sizeof(strTimeDateBuf), "%Y-%m-%d %H:%M:%S", &tmBuf);

    const std::time_t cutTime = nTime;

    std::string baseDateTime(strTimeDateBuf);
    /* 返回空格前的日期2025-09-20 */
    std::string strDate = baseDateTime.substr(0, baseDateTime.find(' '));

    /* 从2025-09-20得到如下格式日期：20250920 */
    strDate.erase(std::remove(strDate.begin(), strDate.end(), '-'), strDate.end());
    
    /* 拼接出完整的m3u8路径，如：/opt/course/record/20250920/normal_20250920.m3u8 */
    filePath = std::string(RECORD_PATH) + "/" + strDate  + "/normal_" + strDate + ".m3u8";

    std::ifstream in(filePath, std::ios::in);
    if (!in.is_open()) 
    {
        return -1;
    }

    // 创建临时文件，直接边读边写
    std::string tmpPath = filePath.string() + ".tmp";
    std::ofstream out(tmpPath, std::ios::out | std::ios::trunc);
    if (!out.is_open()) 
    {
        return -1;
    }

    std::string line;
    // 缓存上一行，处理 EXTINF 和 DATE-TIME 的关联性
    std::string lastLine; 
    bool stopCollect = false;
    bool seenEndList = false;
    const std::string TAG_DATE_TIME = "#EXT-X-PROGRAM-DATE-TIME:";

    while (std::getline(in, line)) 
    {
        // 移除换行符
        if (!line.empty() && line.back() == '\r') line.pop_back();

        if (!stopCollect) 
        {
            if (line.compare(0, TAG_DATE_TIME.length(), TAG_DATE_TIME) == 0) 
            {
                time_t rowTime = fastParseTime(line);
                if (rowTime > nTime) 
                {
                    stopCollect = true;
                }
            }
            
            if (!stopCollect) 
            {
                out << line << "\n";
            }
        }

        // 停止收集，寻找结束标签
        if (stopCollect) 
        {
            if (line.find("#EXT-X-ENDLIST") != std::string::npos) 
            {
                out << "#EXT-X-ENDLIST\n";
                seenEndList = true;
                break;
            }
        }
    }

    // 强制补齐结束标签，防止文件损坏
    if (!seenEndList && stopCollect) 
    {
        out << "#EXT-X-ENDLIST\n";
    }

    in.close();
    out.close();

    // 原子替换
    std::error_code ec;
    fs::rename(tmpPath, filePath, ec);
    if (ec) 
    {
        dlog_error("Rename failed: %s", ec.message().c_str());
        fs::remove(tmpPath, ec);
        return -1;
    }

    return 0;
}

static int del_recordFileManageTableInfo(time_t nTime)
{
    dlog_info("同步删除record_file_manage表格");
    char out[32] = {0};
    std::tm tmBuf{};

    localtime_r(&nTime, &tmBuf);
    std::strftime(out, sizeof(out), "%Y-%m-%d %H:%M:%S", &tmBuf);

    Event::RetrievalCond_S stCond;
    stCond.strTime = out;

    stCond.strFilename = "normal_" + TimeUtils_NS::get_currentDate() + ".m3u8";
    RecordFileManage::instance()->del(stCond, RECORD_FILE_TABLE_NAME);

    return 0;
}

/**
 * @brief 安全删除.trash目录，使用低I/O优先级避免影响系统性能
 */
static int del_trashDirectory() 
{
    std::string strTrashDirPath = std::string(RECORD_PATH) + "/.trash";
    
    try 
    {
        if (!fs::exists(strTrashDirPath)) 
        {
            dlog_error("目录不存在: %s",strTrashDirPath.c_str());
            return -1;
        }
    } 
    catch (const fs::filesystem_error& e) 
    {
        const char* msg = e.what();
        if (msg) 
        {
            dlog_error("检查目录时发生错误: %s", msg);
        }
        
        return -1;
    }
    
    dlog_info("正在删除目录: %s",strTrashDirPath.c_str());
    
    std::stringstream cmd;
    cmd << "ionice -c3 rm -rf '" << strTrashDirPath << "'";
    
    // 执行删除命令
    int result = system(cmd.str().c_str());
    
    if (result == 0) 
    {
        dlog_info("目录删除成功: ", strTrashDirPath.c_str());
        return 0;
    } 
    else 
    {
        dlog_error("删除目录失败%s - 返回值:%d ",strTrashDirPath.c_str(),result);
        return -1;
    }
}

int RecordFileManage::formatSDCardSyncRecordDb()
{
    std::lock_guard<std::mutex> lock(m_eventMutex);
    std::vector<std::string> strTables;
    int nRet = 0;
    /* 获取数据库所有表名 */
    strTables = RecordFileDatabase::instance()->get_all_tables();
    if(strTables.size() < 2)
    {
        dlog_error("没有查询到录制文件表格")
        return -1;
    }
    /* 排序（升序） */
    std::sort(strTables.begin(), strTables.end());

    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&t);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d");
    std::string strCurDate = oss.str();

    /* 遍历并比较 */
    for(unsigned int i = 0; i < strTables.size(); i++)
    {
        const auto& table = strTables.at(i);
        if ( (table != RECORD_FILE_TABLE_NAME) && (table != RECORD_DIR_INFO_TABLE_NAME) && (strCurDate != table))
        {
            RecordFileDatabase::instance()->del_table(table);
        }
        if(strCurDate == table)
        {
            RecordFileDatabase::instance()->clear_table(strCurDate);
        }
    }

    RecordFileDatabase::instance()->clear_table(RECORD_FILE_TABLE_NAME);

    Record_NS::RecordDirInfo_S stDirInfo;
    stDirInfo.nChnId = 0;
    nRet = RecordFileDatabase::instance()->get_itemInfo(stDirInfo);

    stDirInfo.nTotalSize = 0;
    stDirInfo.nCount = 0;

    if(nRet < 0)
    {
        RecordFileDatabase::instance()->add(stDirInfo);
    }
    else 
    {
        RecordFileDatabase::instance()->update(stDirInfo);
    }

   EventDatabase::instance()->clear_table(EVENT_TABLE_NAME);

    return 0;
}

void RecordFileManage::dealTimeChange(time_t nTime)
{
    /* 同步m3u8文件 */
    truncateM3U8(nTime);
    /* 删除record_file_manage.db里面记录相关录制文件的信息的表格 */
    del_recordfilemanageDbInfo(nTime);
    /* 删除record_file_manage.db里面record_file_manage表格记录的m3uu8信息 */
    del_recordFileManageTableInfo(nTime);
    /* 删除event_manage.db里面相关事件录制文件信息 */
    del_eventmanageDbInfo(nTime);
    /* 删除录制的ts文件 */
    rm_recordTsFile(RECORD_PATH, nTime);
    /* 恢复录制状态 */
    CRecordCtrl::instance()->start_record();
    /* 删除录制的目录 */
    rm_recordDir(RECORD_PATH, nTime);

    return ;
}

void RecordFileManage::setLoopWrite(bool bLoopWrite)
{
    m_bLoopWriteFlag.store(bLoopWrite, std::memory_order_release);
    return ;
} 

void RecordFileManage::record_file_manage_thread() 
{
    pthread_setname_np(pthread_self(), "RecordFile");
    TimeUtils_NS::TimeJumpCheck_S stTimeJumpCheck;

    stTimeJumpCheck.init();
    double dTimeDiff;
    while(m_bRun.load(std::memory_order_acquire))
    {
        if(m_bLoopWriteFlag)
        {
            int success = del_trashDirectory();
            if(success == 0)  //删除成功
            {
                //更新记录的录制目录大小 
                Record_NS::RecordDirInfo_S stRecordDirInfo;
                long long llSize = 0;
                stRecordDirInfo.nChnId = 0;
                CStorageManage::instance()->get_directory_size(llSize, RECORD_PATH);
                int nRet = RecordFileDatabase::instance()->get_itemInfo(stRecordDirInfo);
                stRecordDirInfo.nTotalSize = llSize;
                if(nRet < 0)
                {
                    RecordFileDatabase::instance()->add(stRecordDirInfo);
                }
                else 
                {
                    RecordFileDatabase::instance()->update(stRecordDirInfo);
                }
                dlog_info("录制目录大小 [%lld] byte", llSize);
            }
            else
            {
                dlog_info("进入循环录制");
                loop_write();
            }
            
            m_bLoopWriteFlag.store(false, std::memory_order_release);
        }
        
        dTimeDiff = stTimeJumpCheck.probe();
        if(dTimeDiff != 0.0)
        {
            dlog_info("时间发生跳变 %lf", dTimeDiff);
            CRecordCtrl::instance()->stop_record();
            /* 等待录制进程停止操作m3u8文件 */
            sleep(1);
            time_t nCurTime = time(NULL);
            dealTimeChange(nCurTime);
        }
        // else if(dTimeDiff > 0)
        // {
        //     dlog_info("时间发生跳变 %lf", dTimeDiff);
        //     CRecordCtrl::instance()->stop_record();
        //     /* 等待录制进程停止操作m3u8文件 */
        //     sleep(1);
        //     CRecordCtrl::instance()->start_record();
        // }

        usleep(500 * 1000);
    }
    return ;
}