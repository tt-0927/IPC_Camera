/**
 * @FilePath     : capture_ctrl.cpp
 * @Author       : 梁浩尧 lianghaoyao@kfb.cn
 * @Date         : 2025-07-17 17:44:26
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-16 14:32:44
 * @Description  : 抓图计划管理
 */

#include <unistd.h>
#include <thread>
#include <sys/time.h>
#include <ctime>
#include <filesystem>
#include "capture_ctrl.h"
#include "capture_configure.h"
#include "capture_database.h"
#include "storage_manage.h"
#include "time_utils.h"
#include <regex>

namespace fs = std::filesystem;

// #define VENC_CHN_JPEG 2
#define DELETE_FILE_TIME_THRESHOLD 60 /* 10分钟 */

/**
 * @brief   : 时间单位转换
 * @param    {TimeUnit_E} eTimeUnit 时间单位
 * @param    {unsigned int} uInterval 时间
 * @return   {unsigned long long} 转换结果 单位毫秒
 */
static unsigned long long time_unit_conversion(Capture_NS::TimeUnit_E eTimeUnit, unsigned int uInterval);

CCaptureCtrl::CCaptureCtrl()
{
}

CCaptureCtrl::~CCaptureCtrl()
{
}

IpcRet_E CCaptureCtrl::init()
{
    /* 更新抓图计划 */
    update_capturePlan();
    /* 更新抓图参数 */
    update_captureParam();

    m_mapEventCaptureStates.clear();
    m_bRun.store(true, std::memory_order_release);
    std::thread tid;
    tid = std::thread(&CCaptureCtrl::run, this);
    tid.detach();

    return OK;
}

IpcRet_E CCaptureCtrl::deinit()
{
    m_bRun.store(false, std::memory_order_release);

    return OK;
}

int CCaptureCtrl::get_jpegVencParamCallback(const GetjpegVencParamCallback &callback)
{
    m_GetJpegVencParamCallback = callback;
    return OK;
}

void CCaptureCtrl::update_capturePlan()
{
    dlog_info("更新抓图计划");
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stop = false;
    m_infos.clear();

    /* 取出所有抓图计划 */
    Capture_NS::CapturePlan_S stWeeklyCapturePlan;
    CCaptureConfigure::instance()->get_configure(stWeeklyCapturePlan);

    Info_S stInfo;

    for (auto &daySchedule : stWeeklyCapturePlan.vstDaySchedules)
    {
        stInfo.nDayOfWeek = (int) daySchedule.enDayOfWeek;
        stInfo.captureTimes = daySchedule.captureTimes;
        m_infos.push_back(stInfo);
    }
    return;
}

void CCaptureCtrl::update_captureParam()
{
    dlog_info("更新抓图参数");
    std::lock_guard<std::mutex> lock(m_mutex);
    CCaptureConfigure::instance()->get_configure(m_captureParams);
    return;
}

void CCaptureCtrl::get_captureParam(Capture_NS::CaptureParam_S &stCaptureParams)
{
    dlog_info("获取抓图参数");
    std::lock_guard<std::mutex> lock(m_mutex);
    unsigned int unWidth;
    unsigned int unHeight;
    unsigned int nUqFactor;

    CCaptureConfigure::instance()->get_configure(stCaptureParams);

    m_GetJpegVencParamCallback(unWidth, unHeight, nUqFactor);

    stCaptureParams.stCaptureEventConfig.stVideoResolution.nWidth = unWidth;
    stCaptureParams.stCaptureEventConfig.stVideoResolution.nHeight = unHeight;
    stCaptureParams.stCaptureEventConfig.enImageQuality = (nUqFactor >= 0 && nUqFactor < 33) ? Capture_NS::LOW
                                                          : (nUqFactor < 66)                 ? Capture_NS::MEDIUM
                                                                                             : Capture_NS::HIGH;

    stCaptureParams.stCaptureTimingConfig.stVideoResolution.nWidth = unWidth;
    stCaptureParams.stCaptureTimingConfig.stVideoResolution.nHeight = unHeight;
    stCaptureParams.stCaptureTimingConfig.enImageQuality = (nUqFactor >= 0 && nUqFactor < 33) ? Capture_NS::LOW
                                                           : (nUqFactor < 66)                 ? Capture_NS::MEDIUM
                                                                                              : Capture_NS::HIGH;

    return;
}

void CCaptureCtrl::start_capture()
{
}

void CCaptureCtrl::stop_capture()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_mapEventCaptureStates.clear();
    m_TimingCaptureFlag = false;
    return;
}

bool CCaptureCtrl::get_event_capture_status()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    /* 只要有一个事件在抓图就返回true */
    for (auto &pair : m_mapEventCaptureStates)
    {
        if (pair.second.bCaptureFlag)
        {
            return true;
        }
    }

    return false;
}

bool CCaptureCtrl::get_event_first_capture_status(const Event::Type_E enType, std::string &strFirstPath)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    /* 当前抓图数量大于0，即完成首张图片 */
    if (m_mapEventCaptureStates[enType].unCaptureCount > 0)
    {
        strFirstPath = m_mapEventCaptureStates[enType].stEventInfo.strVideoPath;
        return true;
    }
    strFirstPath = std::string();
    return false;
}

bool CCaptureCtrl::get_timing_capture_status()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_TimingCaptureFlag;
}

int CCaptureCtrl::set_event_capture(bool bEventEnded, const Event::Info_S &stEventInfo)
{
    /* 没有启用事件抓图 */
    if (!m_captureParams.stCaptureEventConfig.bEnable)
    {
        return ERR;
    }

    if (CStorageManage::instance()->get_SdCardStatus() != SD_CARD_STATUS_E::NORMAL)
    {
        dlog_error("sd卡状态异常，不能进行抓图");
        return ERR;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    Event::Type_E enEventType = stEventInfo.enType;

    /* 事件结束 */
    if (bEventEnded)
    {
        auto it = m_mapEventCaptureStates.find(enEventType);
        if (it != m_mapEventCaptureStates.end())
        {
            /* 停止该事件的抓图 */
            it->second.bCaptureFlag = false;
            it->second.unCaptureCount = 0;
            dlog_info("事件[%d]结束，停止抓图", (int) enEventType);
        }
        /* 置空人脸抓拍文件名 */
        if (enEventType == Event::Type_E::FACE_CAPTURE || enEventType == Event::Type_E::FACE_COMPARE)
        {
            /* 使用独立的锁来清空人脸抓拍文件名 */
            std::lock_guard<std::mutex> faceLock(m_faceMutex);
            m_strFaceCaptureFile = std::string();
        }
        return OK;
    }

    /* 事件开始 */
    /* 时间单位统一转换为ms */
    unsigned long long ullInterval = time_unit_conversion(m_captureParams.stCaptureEventConfig.stTimeInterval.enTimeUnit,
                                                          m_captureParams.stCaptureEventConfig.stTimeInterval.unInterval);

    auto it = m_mapEventCaptureStates.find(enEventType);

    /* 第一次出现该类型事件 */
    if (it == m_mapEventCaptureStates.end())
    {
        EventCaptureState_S stState;
        stState.enType = enEventType;
        stState.bCaptureFlag = true;
        stState.unCaptureCount = 0;
        stState.ullLastCaptureTime = TimeUtils_NS::get_currentTimestampMs();
        stState.stEventInfo = stEventInfo;

        m_mapEventCaptureStates[enEventType] = stState;
        dlog_info("新事件[%d]触发抓图", (int) enEventType);
    }
    else
    {
        /* 判断当前发生该事件类型比上一次发生该事件类型的时间间隔是否大于等于用户设定的抓图时间间隔 */
        unsigned long long ullCurrentTime = TimeUtils_NS::get_currentTimestampMs();
        if (ullCurrentTime - it->second.ullLastCaptureTime >= ullInterval || enEventType == Event::Type_E::FACE_CAPTURE|| enEventType == Event::Type_E::FACE_COMPARE)
        {
            /* 重新开始抓图 */
            it->second.bCaptureFlag = true;
            it->second.unCaptureCount = 0;
            it->second.ullLastCaptureTime = ullCurrentTime;
            it->second.stEventInfo = stEventInfo;
            dlog_info("事件[%d]再次触发抓图", (int) enEventType);
        }
        else
        {
            dlog_debug("事件[%d]时间间隔未达到设定值，不触发抓图", (int) enEventType);
        }
    }

    return OK;
}

int CCaptureCtrl::send_frameData(unsigned char *pData, int nDataLen)
{
    if (!pData || nDataLen <= 0)
    {
        dlog_error("jpeg数据异常");
        return ERR_PARAM_NULL;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    /* 事件抓图 */
    if (m_captureParams.stCaptureEventConfig.bEnable)
    {
        /* 遍历所有事件，对每个正在抓图的事件进行处理 */
        for (auto &pair : m_mapEventCaptureStates)
        {
            Event::Type_E enEventType = pair.first;
            EventCaptureState_S &stState = pair.second;

            if (!stState.bCaptureFlag)
            {
                continue;
            }

            /* 抓图 */
            std::string strFilePath = capture_image(Capture_NS::CaptureType_E::EVENT_CAPTURE, pData, nDataLen, enEventType);
            /* 第一次抓图时，将第一张图片路径更新至事件状态中 */
            if (stState.unCaptureCount == 0)
            {
                stState.stEventInfo.strVideoPath = strFilePath;
                /* 记录人脸抓拍当前全景图片文件名 */
                if (enEventType == Event::Type_E::FACE_CAPTURE|| enEventType == Event::Type_E::FACE_COMPARE)
                {
                    /* 使用独立的锁来设置人脸抓拍文件名 */
                    {
                        std::lock_guard<std::mutex> faceLock(m_faceMutex);
                        m_strFaceCaptureFile = stState.stEventInfo.strVideoPath;
                    }
                    /* 通知一个等待的线程 */
                    m_faceCv.notify_one();
                }
            }
            stState.unCaptureCount++;

            /* 抓图数量达到用户设定的值，停止该事件抓图 */
            if (stState.unCaptureCount >= m_captureParams.stCaptureEventConfig.unNumber)
            {
                stState.bCaptureFlag = false;
                stState.unCaptureCount = 0;
                dlog_info("事件[%d]抓图数量达到设定值[%u]，停止抓图",
                          (int) enEventType,
                          m_captureParams.stCaptureEventConfig.unNumber);
            }
        }
    }

    /* 定时抓图 */
    if (m_TimingCaptureFlag)
    {
        /* 根据选择的时间单位统一转化成毫秒 */
        unsigned long long ullInterval = time_unit_conversion(m_captureParams.stCaptureTimingConfig.stTimeInterval.enTimeUnit,
                                                              m_captureParams.stCaptureTimingConfig.stTimeInterval.unInterval);

        // note 将间隔时间降低50ms，允许误差50ms，避免1999 >= 2000这种情况
        ullInterval -= 50;
        long long llCurrentTime = TimeUtils_NS::get_currentTimestampMs();

        /* 大于等于设定的抓图的时间间隔，进行抓图 */
        if (llCurrentTime - m_lastTimingCaptrueTime >= ullInterval)
        {
            m_lastTimingCaptrueTime = llCurrentTime;
            /* 抓图 */
            capture_image(Capture_NS::CaptureType_E::TIMING_CAPTURE, pData, nDataLen);
        }
    }

    return OK;
}

int CCaptureCtrl::write_to_file(std::string filePath, unsigned char *pData, int nDataLen)
{
    if (!pData || nDataLen <= 0)
    {
        return ERR;
    }

    std::ofstream file(filePath, std::ios::binary | std::ios::app);
    if (!file.is_open())
    {
        dlog_error("打开抓图[%s]文件时出错", filePath.c_str());
        return ERR;
    }

    /* 写入图片数据 */
    file.write(reinterpret_cast<const char *>(pData), nDataLen);

    /* 刷新缓冲区 */
    file.flush();

    /* 获取当前文件大小（写指针位置） */
    std::streampos fileSize = file.tellp();

    file.close();

    /* 强制将文件数据同步到物理磁盘 */
    int fd = ::open(filePath.c_str(), O_WRONLY);
    if (fd != -1)
    {
        ::fsync(fd);
        ::close(fd);
    }

    return static_cast<int>(fileSize); /* 返回文件大小（字节） */
}

std::string CCaptureCtrl::get_date_storage_path()
{
    /* 获取当前日期，格式：20251103 */
    std::string strDate = TimeUtils_NS::get_currentDate(); /* 假设返回格式为"2025-11-03" */

    /* 去掉日期中的'-' */
    std::string strDateFolder;
    for (char c : strDate)
    {
        if (c != '-')
        {
            strDateFolder += c;
        }
    }

    /* 构建完整路径 */
    std::string strPath = std::string(CAPTURE_PATH) + "/" + strDateFolder;

    return strPath;
}

bool CCaptureCtrl::ensure_directory_exists(const std::string &path)
{
    struct stat info;

    /* 检查目录是否存在 */
    if (stat(path.c_str(), &info) != 0)
    {
        /* 目录不存在，创建目录 */
        if (mkdir(path.c_str(), 0755) != 0)
        {
            dlog_error("创建目录失败: %s", path.c_str());
            return false;
        }
        dlog_info("创建目录成功: %s", path.c_str());
    }
    else if (!(info.st_mode & S_IFDIR))
    {
        /* 路径存在但不是目录 */
        dlog_error("路径存在但不是目录: %s", path.c_str());
        return false;
    }

    return true;
}

std::string CCaptureCtrl::get_face_capture_file()
{
    std::unique_lock<std::mutex> lock(m_faceMutex);

    /* 等待指定时间，如果超时返回空字符串 */
    if (m_faceCv.wait_for(lock,
                          std::chrono::seconds(3),
                          [this]()
                          {
                              return !m_strFaceCaptureFile.empty();
                          }))
    {
        return m_strFaceCaptureFile;
    }

    return "";
}

std::string CCaptureCtrl::capture_image(Capture_NS::CaptureType_E eCaptureType,
                                        unsigned char *pData,
                                        int nDataLen,
                                        Event::Type_E enEventType)
{
    std::string strFilePath;
    if (!pData || nDataLen <= 0)
    {
        // dlog_error("图片数据为空");
        return strFilePath;
    }

    /* 获取按日期分类的存储路径 */
    std::string strStoragePath = get_date_storage_path();

    /* 确保目录存在 */
    if (!ensure_directory_exists(strStoragePath))
    {
        dlog_error("确保目录存在失败");
        return strFilePath;
    }

    /*
     * send_frameData() 之外也有直接调用抓图的入口。在格式化或SD卡异常时
     * 必须在创建目录和访问抓图数据库之前退出。
     */
     if (CStorageManage::instance()->get_SdCardStatus() != SD_CARD_STATUS_E::NORMAL)
     {
         dlog_warn("SD card is not ready, skip capture");
         return strFilePath;
     }

    int nSize = 0;

    /* 事件抓图 */
    if (eCaptureType == Capture_NS::CaptureType_E::EVENT_CAPTURE)
    {
        auto it = m_mapEventCaptureStates.find(enEventType);
        if (it == m_mapEventCaptureStates.end())
        {
            dlog_error("未找到事件[%d]的抓图状态", (int) enEventType);
            return strFilePath;
        }

        EventCaptureState_S &stState = it->second;
        Event::Info_S &stEventInfo = stState.stEventInfo;

        /* 构建文件路径 */
        strFilePath = strStoragePath + "/" + stEventInfo.strDate + "_" + stEventInfo.strTime + "_" +
                      std::to_string(static_cast<int>(enEventType)) + "_" + std::to_string(stState.unCaptureCount + 1) + ".jpg";

        /* 保存图片 */
        nSize = write_to_file(strFilePath, pData, nDataLen);
        if (nSize < 0)
        {
            dlog_error("写入图片文件失败");
            return strFilePath;
        }

        /* 记录事件图片到数据库 */
        Capture_NS::CaptureInfo_S stInfo;
        stInfo.nChnId = 0;
        stInfo.strImagePath = strFilePath;
        stInfo.nImageSize = nSize;
        stInfo.strStartTime = stEventInfo.strStartTime;
        stInfo.strEndTime = stEventInfo.strEndTime;
        stInfo.enType = enEventType;
        CCaptureDatabase::instance()->add(stInfo);

        /* 更新抓图目录信息 */
        Capture_NS::CaptureDirInfo_S stDirInfo;
        stDirInfo.nChnId = 0;
        int nRet = CCaptureDatabase::instance()->get_itemInfo(stDirInfo);

        stDirInfo.nTotalSize += (long long) nSize;
        stDirInfo.nCount++;

        if (nRet < 0)
        {
            CCaptureDatabase::instance()->add(stDirInfo);
        }
        else
        {
            CCaptureDatabase::instance()->update(stDirInfo);
        }
    }
    else /* 定时抓图 */
    {
        std::string strCurrentDate = TimeUtils_NS::get_currentDate();
        std::string strCurrentTimeMs = TimeUtils_NS::get_currentTimeMs();
        std::string strStartTime = TimeUtils_NS::get_currentDateWithDash() + " " + TimeUtils_NS::get_currentTimeWithColon();
        /* 构建文件路径 */
        strFilePath = strStoragePath + "/" + strCurrentDate + "_" + strCurrentTimeMs + ".jpg";

        /* 保存图片 */
        nSize = write_to_file(strFilePath, pData, nDataLen);
        if (nSize < 0)
        {
            dlog_error("写入图片文件失败");
            return strFilePath;
        }

        /* 记录定时抓图信息进数据库 */
        Capture_NS::CaptureInfo_S stInfo;
        stInfo.nImageSize = nSize;
        stInfo.nChnId = 0;
        stInfo.enType = Event::Type_E::SCREENSHOT;
        stInfo.strStartTime = strStartTime;
        stInfo.strEndTime = strStartTime;
        stInfo.strImagePath = strFilePath;

        CCaptureDatabase::instance()->add(stInfo);

        /* 更新抓图目录信息 */
        Capture_NS::CaptureDirInfo_S stDirInfo;
        stDirInfo.nChnId = 0;

        int nRet = CCaptureDatabase::instance()->get_itemInfo(stDirInfo);
        stDirInfo.nTotalSize += (long long) nSize;
        stDirInfo.nCount++;

        if (nRet < 0)
        {
            CCaptureDatabase::instance()->add(stDirInfo);
        }
        else
        {
            CCaptureDatabase::instance()->update(stDirInfo);
        }
    }
    return strFilePath;
}

int CCaptureCtrl::delete_old_images()
{
    dlog_info("开始批量删除旧图片");

    /* 检查TF卡是否存在 */
    if (CStorageManage::instance()->get_SdCardStatus() != SD_CARD_STATUS_E::NORMAL)
    {
        dlog_error("TF卡状态异常，无法删除图片");
        return ERR;
    }

    /* 获取抓图目录信息 */
    Capture_NS::CaptureDirInfo_S stDirInfo;
    stDirInfo.nChnId = 0;
    int nRet = CCaptureDatabase::instance()->get_itemInfo(stDirInfo);
    if (nRet < 0)
    {
        dlog_info("抓图目录信息不存在，无需删除");
        return OK;
    }

    /* 构建查询条件：通道ID = 0，按时间升序排序，限制查询数量 */
    Db::MatchMethods methods;

    /* 查询条件：通道ID = 0 */
    methods.push_back(Db::MatchMethod(Db::Element(Db::INFO_CHNL_ID, 0), Db::FIND_CRITERION_EQ, Db::FIND_CRITERION_NONE));

    /* 按开始时间升序排序（从旧到新） */
    std::string strOrderKey = "order by " + std::string(Db::INFO_CAPTURE_STATRTIME);
    std::string strOrderValue = "asc";
    methods.push_back(Db::MatchMethod(Db::Element(strOrderKey, strOrderValue), Db::FIND_CRITERION_NONE, Db::FIND_CRITERION_NONE));

    /* 限制查询数量为 BATCH_DELETE_COUNT */
    std::string strLimitKey = "limit";
    methods.push_back(
        Db::MatchMethod(Db::Element(strLimitKey, BATCH_DELETE_COUNT), Db::FIND_CRITERION_NONE, Db::FIND_CRITERION_NONE));

    /* 查询最旧的图片记录（已在数据库层面排序和限制数量） */
    std::vector<Capture_NS::CaptureInfo_S> vOldImages;
    nRet = CCaptureDatabase::instance()->find(methods, vOldImages);
    if (nRet < 0)
    {
        dlog_error("查询旧图片记录失败");
        return ERR;
    }

    if (vOldImages.empty())
    {
        dlog_info("没有找到可删除的图片记录");
        return OK;
    }

    dlog_info("查询到 %zu 条旧图片记录，准备删除", vOldImages.size());

    /* 批量删除 */
    int nDeleteCount = 0;
    long long llDeletedSize = 0;
    std::string strStartTime = std::string();
    for (const auto &stInfo : vOldImages)
    {
        /* 删除图片文件 */
        if (remove(stInfo.strImagePath.c_str()) != 0)
        {
            dlog_error("删除图片失败: %s, error: %s", stInfo.strImagePath.c_str(), strerror(errno));
        }
        llDeletedSize += stInfo.nImageSize;
        nDeleteCount++;

        if (stInfo.strStartTime > strStartTime)
        {
            strStartTime = stInfo.strStartTime;
        }
    }

    /* 删除数据库记录 - 删除事件数据库中符合小于等于strStartTime条件的数据 */
    MatchMethods delMethods;
    Event::RetrievalCond_S stCond;
    stCond.strStartTime = strStartTime;
    delMethods.push_back(
        MatchMethod(Element(INFO_CAPTURE_STATRTIME, stCond.strStartTime), FIND_CRITERION_IE, FIND_CRITERION_AND));
    if (delMethods.size() != 0)
    {
        MatchMethod &lastMethod = delMethods.back();
        lastMethod.enAndOr = FIND_CRITERION_NONE;
    }
    nRet = CCaptureDatabase::instance()->del(delMethods, CAPTURE_TABLE_NAME);
    if (nRet < 0)
    {
        dlog_error("删除数据库记录失败");
    }

    /* 更新抓图目录信息 */
    if (nDeleteCount > 0)
    {
        stDirInfo.nTotalSize -= llDeletedSize;
        stDirInfo.nCount -= nDeleteCount;

        /* 确保不为负数 */
        if (stDirInfo.nTotalSize < 0)
            stDirInfo.nTotalSize = 0;
        if (stDirInfo.nCount < 0)
            stDirInfo.nCount = 0;

        CCaptureDatabase::instance()->update(stDirInfo);
        dlog_info("批量删除完成，共删除 %d 张图片，释放空间 %lld Bytes 更新后数据表空间应为： %lld",
                  nDeleteCount,
                  llDeletedSize,
                  stDirInfo.nTotalSize);
    }

    /* 清空vector释放内存 */
    vOldImages.clear();
    vOldImages.shrink_to_fit();

    return OK;
}

static unsigned long long time_unit_conversion(Capture_NS::TimeUnit_E eTimeUnit, unsigned int uInterval)
{
    unsigned int uTimeInterval = 1000;
    switch (eTimeUnit)
    {
    case Capture_NS::TimeUnit_E::SECONDS:
        uTimeInterval = uInterval * 1000;
        break;
    case Capture_NS::TimeUnit_E::MINUTES:
        uTimeInterval = uInterval * 60 * 1000;
        break;
    case Capture_NS::TimeUnit_E::HOURS:
        uTimeInterval = uInterval * 60 * 60 * 1000;
        break;
    case Capture_NS::TimeUnit_E::DAYS:
        uTimeInterval = uInterval * 60 * 60 * 24 * 1000;
        break;
    case Capture_NS::TimeUnit_E::MILLISECONDS:
    default:
        uTimeInterval = uInterval;
        break;
    }
    return static_cast<unsigned long long>(uTimeInterval);
}

/* 时间字符串转时间戳 */
time_t strToTimestamp(const std::string &timeStr)
{
    std::tm tm{};
    strptime(timeStr.c_str(), "%Y%m%d_%H%M%S", &tm);
    return mktime(&tm);
}

/* 遍历目录并比较时间 */
static void checkFilesBySuffix(const std::string &strDirPath, const std::string &strSuffix)
{
    if (!std::filesystem::exists(strDirPath))
    {
        return; // 不存在
    }

    if (!std::filesystem::is_directory(strDirPath))
    {
        return; // 存在但不是目录（是文件等）
    }

    std::regex pattern(R"((\d{8}_\d{6}))");

    time_t now = time(nullptr);

    for (const auto &entry : fs::directory_iterator(strDirPath))
    {
        if (!entry.is_regular_file())
        {
            continue;
        }

        std::string filename = entry.path().filename().string();

        /* 判断后缀 */
        if (entry.path().extension() != strSuffix)
        {
            continue;
        }

        std::smatch match;

        if (std::regex_search(filename, match, pattern))
        {
            std::string timeStr = match[1]; // 20260312_112817

            time_t fileTime = strToTimestamp(timeStr);

            long lDiff = now - fileTime;

            if (lDiff >= DELETE_FILE_TIME_THRESHOLD)
            {
                fs::path strRmFilePath = strDirPath + "/" + filename;

                std::string strCmd = "rm -rf " + strRmFilePath.string();
                system(strCmd.c_str());

                dlog_info("cmd [%s]", strCmd.c_str());
            }
        }
    }
}

void CCaptureCtrl::run()
{
    pthread_setname_np(pthread_self(), "CapCtrlRun");

    m_lastTimingCaptrueTime = TimeUtils_NS::get_currentTimestampMs();

    /* 循环抓图检查计数器（避免频繁检查） */
    int nLoopCheckCounter = 0;
    const int LOOP_CHECK_INTERVAL = 50; /* 每50次循环检查一次（约10秒） */

    while (m_bRun.load(std::memory_order_acquire))
    {
        /* 避免长时间占用cpu */
        usleep(200 * 1000);

        /* 检查sd卡状态 */
        if (CStorageManage::instance()->get_SdCardStatus() != SD_CARD_STATUS_E::NORMAL)
        {
            continue;
        }

        /* 循环抓图功能：定期检查TF卡配额 */
        nLoopCheckCounter++;
        if (nLoopCheckCounter >= LOOP_CHECK_INTERVAL)
        {
            nLoopCheckCounter = 0;

            /* 检查是否达到配额空间大小 */
            if (CStorageManage::instance()->get_captureDirUseStatus() < 0)
            {
                dlog_info("达到配额空间，执行循环抓图删除操作");
                /* 删除旧图片 */
                delete_old_images();
            }

            /* 检查下载压缩包文件 */
            fs::path strDirPath = std::string(CAPTURE_PATH) + "/tmp";
            checkFilesBySuffix(strDirPath, ".tgz");
        }

        /*当前星期几*/
        int nDayOfWeek = TimeUtils_NS::getTodayDayOfWeek();
        /*自当天开始的秒数*/
        int nCurrentTime = TimeUtils_NS::getSecondsSinceStartOfDay();

        std::lock_guard<std::mutex> lock(m_mutex);

        /* 开启定时抓图计划 */
        if (m_captureParams.stCaptureTimingConfig.bEnable)
        {
            /* 取出其中一天的抓图计划（定时抓图） */
            for (const auto &stInfo : m_infos)
            {
                if (stInfo.nDayOfWeek != nDayOfWeek)
                {
                    continue;
                }

                /* 是否应该抓图 */
                bool shouldCapture = false;

                /* 遍历当天的所有抓图时间段 */
                for (const auto &captureTime : stInfo.captureTimes)
                {
                    if (nCurrentTime >= captureTime.nStartTime && nCurrentTime <= captureTime.nEndTime)
                    {
                        m_TimingCaptureFlag = true;
                        shouldCapture = true;
                        break;
                    }
                }

                /* 当前时间点不在抓图计划中 */
                if (!shouldCapture)
                {
                    m_TimingCaptureFlag = false;
                    continue;
                }
            }
        }
        else
        {
            m_TimingCaptureFlag = false;
        }
    }
}
