/**
 * @FilePath     : event_abnormal_detector.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-06-04
 * @Description  : 异常报警检测器实现
 */

#include "event_abnormal_detector.h"

#include <pthread.h>

#include "event_linkage.h"
#include "event_configure.h"
#include "system_monitor.h"
#include "dlog.h"
#include "IpcRet.h"
#include "wifi_manage.h" 
#include "4g_manage.h" 
CAbnormalDetector::CAbnormalDetector() = default;

CAbnormalDetector::~CAbnormalDetector()
{
    deinit();
}

int CAbnormalDetector::init()
{
    /* 从持久化配置文件中加载各异常类型的联动配置 */
    std::set<Alarm::AbnormalDetection_S> setConfigs;
    CEventConfigure::instance()->get_configure(setConfigs);

    {
        std::lock_guard<std::mutex> lock(m_configMutex);
        for (const auto &stConfig : setConfigs)
        {
            m_mapLinkageConfig[stConfig.enAbnormalType] = stConfig.stLinkageList;
        }
    }

    dlog_info("异常检测器配置加载完成，已加载 %zu 个异常类型的联动配置", setConfigs.size());

    /* 启动检测线程，线程将持续运行直到进程退出 */
    m_bRunning.store(true);
    m_thread = std::thread(&CAbnormalDetector::detection_loop, this);

    return OK;
}

int CAbnormalDetector::deinit()
{
    if (!m_bRunning.load())
    {
        return OK;
    }
    /* 通知检测线程退出并等待线程结束 */
    m_bRunning.store(false);
    if (m_thread.joinable())
    {
        m_thread.join();
    }
    dlog_info("异常检测器已反初始化");

    return OK;
}

void CAbnormalDetector::update_linkage_config(Alarm::AbnormalType_E enType,
                                               const Alarm::LinkageList_S &stLinkageList)
{
    std::lock_guard<std::mutex> lock(m_configMutex);
    m_mapLinkageConfig[enType] = stLinkageList;

    dlog_info("更新异常检测联动配置: 类型=%d, tradition=%zu, alarmOutput=%zu",
              static_cast<int>(enType),
              stLinkageList.tradition.size(),
              stLinkageList.alarmOutput.size());
}

void CAbnormalDetector::detection_loop()
{
    /* 设置线程名称，便于调试和 ps 查看 */
    pthread_setname_np(pthread_self(), "AbnormalDet");
    dlog_info("异常检测轮询线程已启动，检测间隔=%d秒", DETECTION_INTERVAL_SEC);

    /* 缓存系统监控器单例引用，避免每次循环重复获取 */
    auto *pMonitor = CSystemMonitor::instance();

    while (m_bRunning.load())
    {
        /* 一次性获取当前系统状态快照，减少重复查询开销 */
        CSystemMonitor::SystemStatus stStatus = pMonitor->get_current_status();
        #if CAP_NETWORK_WIFI
        CWifiManager::instance()->isWifiConnectedAndWiredDisconnected(!stStatus.networkConnected);
        #endif
        #if CAP_NETWORK_4G
        FourGManager::instance()-> updateRouteIfNeeded(!stStatus.networkConnected);
        #endif
        /* 遍历全部 5 种异常类型（DISK_FULL=0 到 ILLEGAL_ACCESS=4） */
        for (int i = 0; i < ABNORMAL_TYPE_COUNT; ++i)
        {
            /* 每种类型检测前先检查退出标志，确保线程能及时响应停止请求 */
            if (!m_bRunning.load())
            {
                break;
            }

            auto enType = static_cast<Alarm::AbnormalType_E>(i);

            /* 检查该类型是否处于异常状态 */
            if (!check_abnormal_status(enType, stStatus))
            {
                continue;
            }

            /* 将异常类型映射为事件框架的事件类型 */
            Event::Type_E enEventType = map_to_event_type(enType);
            if (enEventType == Event::Type_E::UNKNOWN)
            {
                continue;
            }

            /* 异常已确认，触发事件处理（记录日志 + 按配置执行联动） */
            trigger_event(enType, enEventType);
        }

        /* 等待下一个检测周期 */
        std::this_thread::sleep_for(std::chrono::seconds(DETECTION_INTERVAL_SEC));
    }

    dlog_info("异常检测轮询线程已退出");
}

bool CAbnormalDetector::check_abnormal_status(Alarm::AbnormalType_E enType,
                                               const CSystemMonitor::SystemStatus &stStatus)
{
    /* 根据异常类型检查对应的系统状态指标 */
    switch (enType)
    {
    case Alarm::AbnormalType_E::DISK_FULL:
        /* 磁盘使用率超过 95% 视为磁盘满 */
        return stStatus.diskUsage >= 95.00;
    case Alarm::AbnormalType_E::DISK_ERROR:
        /* 磁盘错误标志为 true 表示磁盘异常 */
        return stStatus.diskErrorFlag;
    case Alarm::AbnormalType_E::NET_BROKEN:
        /* 网络未连接表示网络断开 */
        return !stStatus.networkConnected;
    case Alarm::AbnormalType_E::IP_CONFLICT:
        /* IP 冲突标志为 true 表示存在 IP 冲突 */
        return stStatus.ipConflictFlag;
    case Alarm::AbnormalType_E::ILLEGAL_ACCESS:
        /* 未授权访问尝试次数大于 0 表示存在非法访问 */
        return stStatus.unauthorizedAccessAttempts > 0;
    default:
        dlog_warn("未知的异常类型: %d", static_cast<int>(enType));
        return false;
    }
}

Event::Type_E CAbnormalDetector::map_to_event_type(Alarm::AbnormalType_E enType)
{
    /* 将异常报警的子类型映射为事件框架的统一事件类型 */
    switch (enType)
    {
    case Alarm::AbnormalType_E::DISK_FULL:
        return Event::Type_E::DISK_FULL;
    case Alarm::AbnormalType_E::DISK_ERROR:
        return Event::Type_E::DISK_ERROR;
    case Alarm::AbnormalType_E::NET_BROKEN:
        return Event::Type_E::NET_BROKEN;
    case Alarm::AbnormalType_E::IP_CONFLICT:
        return Event::Type_E::IP_CONFLICT;
    case Alarm::AbnormalType_E::ILLEGAL_ACCESS:
        return Event::Type_E::ILLEGAL_ACCESS;
    default:
        dlog_warn("无法映射异常类型到事件类型: %d", static_cast<int>(enType));
        return Event::Type_E::UNKNOWN;
    }
}

void CAbnormalDetector::trigger_event(Alarm::AbnormalType_E enType, Event::Type_E enEventType)
{
    /* 始终记录异常检测日志，无论是否配置了联动 */
    dlog_info("检测到系统异常: 类型=%d, 事件=%d", static_cast<int>(enType), static_cast<int>(enEventType));

    /* 调用事件联动模块处理，由其根据配置决定执行哪些联动动作（日志写入、声音、闪光灯等） */
    CEventLinkage::instance()->handleEvent(enEventType, false);
}
