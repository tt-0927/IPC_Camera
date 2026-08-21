/**
 * @FilePath     : record_ctrl.cpp
 * @Author       : zhangjc (zhangjc@kfb.cn)
 * @Date         : 2024-12-16
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-20 17:13:15
 * @Description  : 录制控制
 */

#include "record_ctrl.h"
#include "system_convert.h"
#include "record_file_manage.h"
#include "record_configure.h"
#include <chrono>
#include <ctime>
#include <unistd.h>
#include <thread>
#include <unordered_map>
#include <sys/statvfs.h>
// #include "ipc_define.h"
#include "system_define.h"
// #include "ipc_manage.h"
// #include "disk_manage.h"
#include "action_code.h"
#include "event_configure.h"
#include "path_define.h"
#include "task_publish.h"
#include "event_linkage.h"
#include "m3u8.h"
#include "storage_manage.h"
#include "event_database_manage.h"
#include "time_utils.h"

/* 循环录制阈值 */
#define SD_DEL_DEFAULT_VALUE_UP 95
#define SD_DEL_DEFAULT_VALUE_DOWN 85

/* 事件录制时长 单位ms */
#define EVNET_RECORD_TIME 20000 

CRecordCtrl::CRecordCtrl()
{

}

CRecordCtrl::~CRecordCtrl()
{
    
}

IpcRet_E CRecordCtrl::init()
{
    update();
    update_advancedParam(); 
    m_bRun.store(true, std::memory_order_release);
    std::thread tid;
    tid = std::thread(&CRecordCtrl::run, this);
    tid.detach();
    RecordFileManage::instance()->init();
    return OK;
}

IpcRet_E CRecordCtrl::deinit()
{
    m_bRun.store(false, std::memory_order_release);
    m_stop = true;
    RecordFileManage::instance()->deinit();
    return OK;
}

void CRecordCtrl::update()
{
    m_stop = false;
    dlog_info("更新录制计划");
    m_infos.clear();
    /* 拿出录制计划 */
    Record_NS::Schedule_S schedule;
    schedule.daySchedules.clear();
    RecordConfigure::instance()->get_configure(schedule);

    /* 是否开启录制 */
    if (!schedule.bEnable)
    {
        stop_record();
        /* 不启用录制计划， */
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        Info_S stInfo;
        /* 每天的录制计划 */
        for (auto &daySchedule : schedule.daySchedules)
        {
            stInfo.nDayOfWeek = (int)daySchedule.enDayOfWeek;
            stInfo.recordTimes = daySchedule.recordTimes;
            m_infos.push_back(stInfo);
        }
    }
}

void CRecordCtrl::update_advancedParam()
{
    RecordConfigure::instance()->get_configure(m_advancedParam);
    return ;
}

void CRecordCtrl::start_record()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stop = false;
}

void CRecordCtrl::stop_record()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stop = true;
}

bool CRecordCtrl::is_running() const
{
    return m_bRun.load(std::memory_order_acquire);
}

/* 获取录制状态 */
Record_NS::Status_E CRecordCtrl::get_recordStatus()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_enRecordStatus;
}

int CRecordCtrl::set_humanRecord(Record_NS::Info_S &stInfo)
{
    /* 获取录制信息 */
    int nRet = OK;

    //! 检测存储 后续完成

    /* 填充录制信息 */
    /* 音视频录制 */
    stInfo.nVideoStatus = true;
    stInfo.nAudioStatus = true;
    /* 录制格式 */
    stInfo.nRecordFormat = 0;
    /* 查找字符串存不存在record */
    if (stInfo.path.empty())
    {
        dlog_error("录像路径不正确，不进行录制操作");
        return ERR;
    }
    stInfo.path += "record";
    stInfo.nEventType = (int)Event::Type::HUMAN_RECORD;
    std::string info = Convert::to_string(stInfo);
    CRecordServer::instance()->fill_head(info, AC_CONTROL_RECORD_INFO);
    nRet = CRecordServer::instance()->send(static_cast<const void *>(info.c_str()), info.size() + 1, AC_CONTROL_RECORD_INFO);
    if (nRet < 0)
    {
        dlog_error("发送录像控制命令失败");
        return ERR;
    }
    m_stHumanRecordInfo = stInfo;
    return OK;
}

void CRecordCtrl::get_humanRecord(Record_NS::Info_S &stInfo)
{
    stInfo = m_stHumanRecordInfo;
}

void CRecordCtrl::reset_status()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_enRecordStatus = Record_NS::NO_OPERATION;
}

void CRecordCtrl::set_event_record(bool bEventRecordFlag, Event::Info_S &stEventInfo)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_eventRecordFlag = bEventRecordFlag;
    m_eventInfo = stEventInfo;
    return ;
}

void CRecordCtrl::get_event_record(Event::Info_S &stEventInfo)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    stEventInfo = m_eventInfo;
    return ;
}

void CRecordCtrl::set_event_ts_info(int stEventInfo, std::string &strPath)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_eventInfo.nVideoSize += stEventInfo;
    m_eventInfo.strVideoPath = strPath;
    return ;
}

void CRecordCtrl::fill_info(Record_NS::Info_S &stInfo)
{
    /* 音视频录制 */
    stInfo.nVideoStatus = true;
    stInfo.nAudioStatus = true;
    /* 录制格式 */
    stInfo.nRecordFormat = 0;
    stInfo.strRecordName = "";
    /* 主码流 */
    stInfo.nStreamType = 0;

    stInfo.path = RECORD_PATH;

    // m_RecordScheduleType:1定时录制，2事件录制
    // if(m_RecordScheduleType == 1)
    // {
        stInfo.nEventType = 0;
    // }
    // else if(m_RecordScheduleType == 2)
    // {
    //     stInfo.nEventType = 1;
    // }

#if 0
    /* 获取当前日期 */
    auto date = get_currentDate();

    /* 根据存储模式来构建路径 */
    System::StorageModeInfo_S stStorageModeInfo;
    Convert::read_file(RECORD_STORAGE_MODE_CONFIG_FILE, stStorageModeInfo);
     /* 配额模式 */
    if (stStorageModeInfo.enStorageMode == System::QUOTA)
    {
        /* 获取配额路径 */
        Record_NS::RecordQuotaConfig_S stRecordQuotaConfig;
        stRecordQuotaConfig.nChnId = stInfo.nChnId;
        RecordConfigure::instance()->get_configure(stRecordQuotaConfig);
        if(!stRecordQuotaConfig.strRecordPath.empty())
        {
            stInfo.path = stRecordQuotaConfig.strRecordPath;
             /* 获取当前通道配额大小 */
            int nQuotaSize = 1024 * 1024 * 1024; // 默认1G
            try 
            {
                nQuotaSize = std::stoi(stRecordQuotaConfig.strVideoQuota);
            }
            catch (...)
            {
                dlog_error("配额[%s]大小非法", stRecordQuotaConfig.strVideoQuota);
            }
                /* 获取当前通道已用内存大小 */
            double  dUsedSize = RecordFileManage::instance()->get_channel_size(stInfo.path);

                /* 是否超过配额大小 */
            if(dUsedSize >= nQuotaSize)
            {
                dlog_error("超过录像配额大小，停止录制");
                /* 停止录制操作 */
                stInfo.nRecordStatus = Record_NS::STOP_OPERATION;
            }
        }
        /* 配额大小为空正常获取路径 */
        else
        {
            stInfo.path = CDiskManage::instance()->get_diskRecordPath(stInfo.nChnId);
            if(stInfo.path.empty())
            {
                dlog_error("硬盘已满 停止录制");
                /* 停止录制操作 */
                stInfo.nRecordStatus = Record_NS::STOP_OPERATION;
            }
        }
        
    }
    /* 盘组模式 */
    else if (stStorageModeInfo.enStorageMode == System::DISKGROUP)
    {
        /* 获取盘组配置信息 */
        std::set<Record_NS::RecordDiskGroupConfig_S> stRecordDiskGroupConfigs;
        RecordConfigure::instance()->get_configure(stRecordDiskGroupConfigs);
        int nGroupId = -1;
        /* 获取通道号所在盘组 */
        for (const auto& group : stRecordDiskGroupConfigs) 
        {
            if (std::find(group.vecChannId.begin(), group.vecChannId.end(), stInfo.nChnId) != group.vecChannId.end()) 
            {
                nGroupId = group.nGroupId;
                break; 
            }
        }
        /* 没找到对应盘组 停止录制 */
        if(nGroupId < 0)
        {
            dlog_error("通道[%d]未配置盘组 录制失败",stInfo.nChnId);
            stInfo.nRecordStatus = Record_NS::STOP_OPERATION;
        }
        else
        /* 通过盘号和通道获取录制路径 */
        {
            stInfo.path = CDiskManage::instance()->get_diskGroupPath(stInfo.nChnId,nGroupId);
            if(stInfo.path.empty())
            {
                dlog_error("硬盘已满或未找到配置的盘组 停止录制");
                /* 停止录制操作 */
                stInfo.nRecordStatus = Record_NS::STOP_OPERATION;
            }

            /* 冗余录像 */
            Record_NS::AdvancedParam_S stParam;
            stParam.nChnId = stInfo.nChnId;
            RecordConfigure::instance()->get_configure(stParam);
            /* 获取冗余录像路径 */
            if(stParam.bEnRedundancy)
            {
                stInfo.redunPath = CDiskManage::instance()->get_diskRedundPath(stInfo.nChnId);
            }

        }
    }
    
    /* 一个硬盘 暂时使用默认挂载路径 */
    // stInfo.path = "/opt/course/record/D" + std::to_string(stInfo.nChnId + 1) + "/";
    
    stInfo.strRecordName = "";
    /* 主码流 */
    stInfo.nStreamType = 0;
#endif
    return;
}

int CRecordCtrl::ctrl(int nStatus)
{
    Record_NS::Info_S stInfo;
    stInfo.nChnId = m_ChnId;
    stInfo.nRecordStatus = nStatus;

    //! 检测存储 后续完成

    /* 填充录制信息 */
    fill_info(stInfo);
    /* 音视频录制 */
    stInfo.nVideoStatus = true;
    stInfo.nAudioStatus = true;
    /* 录制格式 */
    stInfo.nRecordFormat = 0;

    /* 停止录制跳过路径判断 */
    if (!m_stop)
    {
        /* 查找字符串存不存在record */
        if (stInfo.path.find("record") == std::string::npos)
        {
            dlog_error("录像路径不正确，不进行录制操作");
            return ERR;
        }
    }
    dlog_debug("录制控制. nStatus:%d", nStatus);
    std::string info = Convert::to_string(stInfo);
    CRecordServer::instance()->fill_head(info, AC_CONTROL_RECORD_INFO);
    if(nStatus == Record_NS::STOP_OPERATION)
    {
        /*记录因存储满停止的时间戳，供事件联动截断endTime使用*/
        m_strLastRecordStopTime = TimeUtils_NS::get_currentDateWithDash() + " " + TimeUtils_NS::get_currentTimeWithColon();
        m_bStoppedDueToStorage = true;
    }
    else if(nStatus == Record_NS::RECORD_OPERATION)
    {
        m_bStoppedDueToStorage = false;
        m_strLastRecordStopTime.clear();
    }
    return CRecordServer::instance()->send(static_cast<const void *>(info.c_str()), info.size() + 1, AC_CONTROL_RECORD_INFO);
}

static int get_sd_usage()
{
    struct statvfs vfs;
    if (statvfs("/", &vfs) != 0) 
    {
        return -1; 
    }

    unsigned long used_blocks = vfs.f_blocks - vfs.f_bavail;
    unsigned long percent = (used_blocks * 100) / vfs.f_blocks;
    
    // 四舍五入
    unsigned long remainder = (used_blocks * 100) % vfs.f_blocks;
    if (remainder * 2 >= vfs.f_blocks) 
    {
        percent++;
    }
    
    // 确保值在合理范围内
    if (percent > 100) 
    {
        return 100;
    }
    return static_cast<int>(percent);
}

int CRecordCtrl::get_RecordScheduleType()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_RecordScheduleType;
}

void CRecordCtrl::set_eventM3u8Path(std::string &strM3u8Path, std::string &strM3u8FileName)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_strEventM3u8Path = strM3u8Path;
    m_strEventM3u8FileName = strM3u8FileName;
    return ;
}

int CRecordCtrl::get_record_status()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    return m_enRecordStatus;
}

int CRecordCtrl::set_record_process_status(bool bRecordProcessStatus)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    dlog_info("record客户端状态:%d", bRecordProcessStatus);
    m_bRecordProcessStatus = bRecordProcessStatus;
    return 0;
}

void CRecordCtrl::update_recordDate()
{
    dlog_debug("日期变更 %s ==> %s\n", m_strCurRecordDate.c_str(), TimeUtils_NS::get_currentDateAndFormat("%Y%m%d").c_str());
	m_strCurRecordDate = TimeUtils_NS::get_currentDateAndFormat("%Y%m%d");
    return ;
}

bool CRecordCtrl::is_newDay()
{
	if (m_strCurRecordDate == TimeUtils_NS::get_currentDateAndFormat("%Y%m%d"))
	{
		return false;
	}
	return true;
}
 
bool CRecordCtrl::isStoppedDueToStorage() const 
{ 
    return m_bStoppedDueToStorage; 
}

const std::string& CRecordCtrl::getLastRecordStopTime() const 
{ 
    return m_strLastRecordStopTime; 
}
    
void CRecordCtrl::run()
{
    pthread_setname_np(pthread_self(), "RecordCtrlRun");

    int nStartRecordTime;
    int nEndRecordTime;

    std::string strEventM3u8Path = std::string();;
    std::string strEventM3u8FileName = std::string();;

    /* 刚启动时sleep 5s 避免record进程还没起来就发送开始录制命令 */
    sleep(5);

    while (m_bRun.load(std::memory_order_acquire))
    {
        usleep(100 * 1000);
        /*当前星期几*/
        int nDayOfWeek = TimeUtils_NS::getTodayDayOfWeek();
        /*自当天开始的秒数*/
        int nCurrentTime = TimeUtils_NS::getSecondsSinceStartOfDay();

        /* record进程还没与stream进程成功建立通信 */
        if(!m_bRecordProcessStatus)
        {
            if (m_enRecordStatus == Record_NS::RECORD_OPERATION)
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_enRecordStatus = Record_NS::STOP_OPERATION;
            }
            // usleep(100 * 1000);
            continue;
        }

        if (m_stop)
        {
            /*无操作/开始录制状态*/
            if (m_enRecordStatus == Record_NS::RECORD_OPERATION || m_enRecordStatus == Record_NS::NO_OPERATION)
            {
                /*停止录制*/
                int nRet = ctrl(Record_NS::STOP_OPERATION);
                if (nRet >= 0)
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    m_enRecordStatus = Record_NS::STOP_OPERATION;
                    dlog_info("结束录制");
                }
            }
            continue;
        }

        if(CStorageManage::instance()->get_SdCardStatus() != SD_CARD_STATUS_E::NORMAL)
        {
            if (m_enRecordStatus == Record_NS::RECORD_OPERATION || m_enRecordStatus == Record_NS::NO_OPERATION)
            { 
                /*停止录制*/
                int nRet = ctrl(Record_NS::STOP_OPERATION);
                if (nRet >= 0)
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    m_enRecordStatus = Record_NS::STOP_OPERATION;
                    dlog_info("sd卡状态异常，结束录制");
                }
            }
            continue;
        }

        /* 当前无录制计划不进行录制 */
        if(m_infos.size() <= 0)
        {
            if (m_enRecordStatus == Record_NS::RECORD_OPERATION)
            {
                /*停止录制*/
                int nRet = ctrl(Record_NS::STOP_OPERATION);
                if (nRet >= 0)
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    m_enRecordStatus = Record_NS::STOP_OPERATION;
                    dlog_info("不存在录制计划或sd卡异常结束录制");
                }
            }
            // usleep(100 * 1000);
            continue;
        }
        if(m_advancedParam.bLoopWrite)
        {
            if(CStorageManage::instance()->get_recordDirUseStatus() < 0)
            {
                RecordFileManage::instance()->setLoopWrite(true);
            }
            
        }
        /* 达到配额空间空间大小 */
        if(!m_advancedParam.bLoopWrite && CStorageManage::instance()->get_recordDirUseStatus() < 0)
        {
            if (m_enRecordStatus == Record_NS::RECORD_OPERATION || m_enRecordStatus == Record_NS::NO_OPERATION)
            {
                /*停止录制*/
                int nRet = ctrl(Record_NS::STOP_OPERATION);
                if (nRet >= 0)
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    m_enRecordStatus = Record_NS::STOP_OPERATION;
                    dlog_info("结束录制");
                }
            }
            // usleep(100 * 1000);
            continue ;
        }

        // 轮询获取当天的录制计划
        for (const auto &stInfo : m_infos)
        {
            if (stInfo.nDayOfWeek != nDayOfWeek)
            {
                continue;
            }
            /*是否应该录制*/
            bool shouldRecord = false;
            // stInfo当天所有计划
            for (auto &recordTime : stInfo.recordTimes)
            {
                // /* 过滤事件录制 */
                // if (recordTime.nType == 1)
                // {
                //     continue;
                // }

                /* 目前定时录制计划才进行 预录以及延时 */
                if(recordTime.nType == 1)
                {
                    nStartRecordTime = recordTime.nStartTime - m_advancedParam.ePreTime;
                    nEndRecordTime = recordTime.nEndTime + m_advancedParam.eDelayTime;
                }
                else 
                {
                    nStartRecordTime = recordTime.nStartTime;
                    nEndRecordTime = recordTime.nEndTime;
                }

                /*判断当前时间，是否在录制计划时间内*/
                if (nCurrentTime >= nStartRecordTime && nCurrentTime < nEndRecordTime)
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    m_RecordScheduleType = recordTime.nType;
                    shouldRecord = true;
                    break;
                }
                else /* 不在录制计划时间内 */
                {
                    if(m_RecordScheduleType != 0)
                    {
                        std::lock_guard<std::mutex> lock(m_mutex);
                        m_RecordScheduleType = 0;
                    }
                }
            }
            /* 如果当前的录制计划不为事件录制计划 */
            if(m_RecordScheduleType != 2)
            {
                /* 停止录制 */
                if (m_stop)
                {
                    shouldRecord = false;
                }

                /*  1 开始 3 结束 */
                if (shouldRecord)
                {
                    /*无操作/停止操作状态*/
                    if (m_enRecordStatus == Record_NS::STOP_OPERATION || m_enRecordStatus == Record_NS::NO_OPERATION)
                    {
                        /*开始录制*/
                        int nRet = ctrl(Record_NS::RECORD_OPERATION);
                        if (nRet >= 0)
                        {
                            std::lock_guard<std::mutex> lock(m_mutex);
                            m_enRecordStatus = Record_NS::RECORD_OPERATION;
                            dlog_info("开始定时录制");
                        }
                    }
                }
                else
                {
                    /*无操作/开始录制状态*/
                    if (m_enRecordStatus == Record_NS::RECORD_OPERATION || m_enRecordStatus == Record_NS::NO_OPERATION)
                    {
                        /*停止录制*/
                        int nRet = ctrl(Record_NS::STOP_OPERATION);
                        if (nRet >= 0)
                        {
                            std::lock_guard<std::mutex> lock(m_mutex);
                            m_enRecordStatus = Record_NS::STOP_OPERATION;
                            dlog_info("结束定时录制");
                        }
                    }
                }
            }
            else /* 事件录制计划 */
            {
                /* 停止录制 */
                if (m_stop)
                {
                    shouldRecord = false;
                }

                /* 当前在事件录制计划事件段内，并且有事件触发 */
                if (shouldRecord && m_eventRecordFlag)
                {
                    /*无操作/停止操作状态*/
                    if (m_enRecordStatus == Record_NS::STOP_OPERATION || m_enRecordStatus == Record_NS::NO_OPERATION)
                    {
                        /*开始录制*/
                        int nRet = ctrl(Record_NS::RECORD_OPERATION);
                        if (nRet >= 0)
                        {
                            dlog_info("开始事件录制");
                            std::lock_guard<std::mutex> lock(m_mutex);
                            m_enRecordStatus = Record_NS::RECORD_OPERATION;
                        }
                    }

                }

                if(CEventLinkage::instance()->get_EventInfoMapSize() == 0)
                {
                    if(m_enRecordStatus == Record_NS::RECORD_OPERATION)
                    {
                        int nRet = ctrl(Record_NS::STOP_OPERATION);
                        if (nRet >= 0)
                        {
                            dlog_info("结束事件录制");
                            std::lock_guard<std::mutex> lock(m_mutex);
                            m_enRecordStatus = Record_NS::STOP_OPERATION;

                            strEventM3u8Path = m_strEventM3u8Path;
                            strEventM3u8FileName = m_strEventM3u8FileName;

                            m_strEventM3u8Path = std::string();
                            m_strEventM3u8FileName = std::string();

                            m_eventRecordFlag = false; 

                            m_eventInfo.strEndTime = TimeUtils_NS::get_currentDateWithDash();
                            m_eventInfo.strEndTime = m_eventInfo.strEndTime + " " + TimeUtils_NS::get_currentTimeAndFormat("%H:%M:%S");
                            EventDatabaseManage::instance()->update(m_eventInfo);
                            m_eventInfo.nVideoSize = 0;
                        }
                    }
                }
            }

        }

        
        /* 等待1s再解析事件录制的m3u8文件，防止录制进程未更新m3u8文件 */
        // if(m_RecordScheduleType == 2) /* 事件录制 */
        // {
        //     if(!strEventM3u8Path.empty() && !strEventM3u8Path.empty())
        //     {
        //         std::lock_guard<std::mutex> lock(m_mutex);
        //         RecordFileManage::instance()->add_eventVideo(strEventM3u8Path, strEventM3u8FileName, m_eventInfo);
        //         m_eventRecordFlag = false; 
        //         strEventM3u8Path = std::string();
        //         strEventM3u8FileName = std::string();
        //     }
        // }

    }
}
