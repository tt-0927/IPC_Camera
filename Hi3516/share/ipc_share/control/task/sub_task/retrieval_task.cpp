/**
 * @file retrieval_task.cpp
 * @author xiejh (xiejh@kfb.cn)
 * @date 2024-10-16
 *
 * @brief 检索任务处理
 */

#include <regex>
#include "retrieval_task.h"
#include "event_convert.h"
#include "record_convert.h"
#include "alarm_convert.h"
#include "common_convert.h"
#include "convert_interface.h"

#include "action_code.h"
#include "record_file_manage.h"
#include "system_define.h"
#include "system_manage.h"
#include "preview_manage.h"
#include "event_linkage.h"
#include "common_define.h"
// #include "AalgorithmManage.h"
#include "path_define.h"
#include "task_publish.h"
#include "log_handler.h"
#include "web_server.h"
#include "storage_manage.h"
#include "time_utils.h"
#include <dirent.h>

/**
 * @brief 日期格式验证 "2024-01-01"
 * @param date
 * @return
 */
static bool isValidDate(const std::string &date)
{
    std::regex dateRegex(R"((\d{4})-(\d{2})-(\d{2}))");
    return std::regex_match(date, dateRegex);
}

/**
 * @brief 时间格式验证 "08:00:00"
 * @param time
 * @return
 */
static bool isValidTime(const std::string &time)
{
    /* 假设时间格式为 HH:MM:SS，使用正则表达式进行简单验证 */
    std::regex timeRegex(R"((\d{2}):(\d{2}):(\d{2}))");
    return std::regex_match(time, timeRegex);
}

/**
 * @brief 日期时间格式验证 "2025-08-07 23:59:59"
 * @param time
 * @return
 */
static bool isValidDateTime(const std::string &datetime)
{
    // 1. 正则表达式验证基本格式
    std::regex datetimeRegex(R"(^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}$)");
    if (!std::regex_match(datetime, datetimeRegex))
    {
        return false;
    }

    // 2. 尝试解析为时间结构
    std::tm            tm = {};
    std::istringstream ss(datetime);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");

    // 3. 检查解析是否成功
    if (ss.fail())
    {
        return false;
    }

    // 4. 验证日期时间组件的合理性
    // 年份：1900-9999（可根据需要调整）
    int year = tm.tm_year + 1900;
    if (year < 1900 || year > 9999)
    {
        return false;
    }

    // 月份：1-12
    if (tm.tm_mon < 0 || tm.tm_mon > 11)
    {
        return false;
    }

    // 日期：1-31（具体验证需要结合月份）
    if (tm.tm_mday < 1 || tm.tm_mday > 31)
    {
        return false;
    }

    // 小时：0-23
    if (tm.tm_hour < 0 || tm.tm_hour > 23)
    {
        return false;
    }

    // 分钟：0-59
    if (tm.tm_min < 0 || tm.tm_min > 59)
    {
        return false;
    }

    // 秒钟：0-59（闰秒为60，但通常不需要）
    if (tm.tm_sec < 0 || tm.tm_sec > 59)
    {
        return false;
    }

    // 5. 验证具体日期的有效性（考虑月份天数和闰年）
    // 各月天数（1月=索引0）
    const int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    // 处理闰年（能被4整除但不能被100整除，或者能被400整除）
    bool isLeapYear = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    int  maxDay     = daysInMonth[tm.tm_mon];

    // 二月闰年处理
    if (tm.tm_mon == 1 && isLeapYear)
    {
        maxDay = 29;
    }

    // 检查日期是否有效
    if (tm.tm_mday > maxDay)
    {
        return false;
    }

    return true;
}

/**
 * @brief 处理时间字符串加上整型秒数"
 * @param timeStr 开始时间字符串
 * @param secondsToAdd 相加的秒数
 * @return 结束时间字符串
 */
static std::string addSeconds(const std::string &timeStr, int add)
{
    int h = 0, m = 0, s = 0;

    if (sscanf(timeStr.c_str(), "%d:%d:%d", &h, &m, &s) != 3)
    {
        return "";
    }

    int total = h * 3600 + m * 60 + s + add;
    total     = ((total % 86400) + 86400) % 86400;

    char buf[9];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", total / 3600, (total % 3600) / 60, total % 60);

    return buf;
}

/**
 * @brief 计算两个日期之间的天数差并打印所有日期"
 * @param start_date 开始日期
 * @param start_date 结束日期
 * @param outDates 中间日期字符串
 * @return
 */
static void calculateAndPrintDays(const std::string &start_date, const std::string &end_date, std::vector<std::string> &outDates)
{
    // 定义时间结构体
    std::tm tm_start = {}, tm_end = {};

    // 解析日期字符串
    std::istringstream ss_start(start_date);
    ss_start >> std::get_time(&tm_start, "%Y-%m-%d");

    std::istringstream ss_end(end_date);
    ss_end >> std::get_time(&tm_end, "%Y-%m-%d");

    // 转换为time_t类型
    std::time_t time_start = std::mktime(&tm_start);
    std::time_t time_end   = std::mktime(&tm_end);

    // 计算天数差
    const int seconds_per_day = 60 * 60 * 24;
    double    difference      = std::difftime(time_end, time_start);
    int       days            = static_cast<int>(difference / seconds_per_day);

    // 打印天数差
    // std::cout << "从 " << start_date << " 到 " << end_date << " 共有 " << days << " 天" << std::endl;
    // std::cout << "日期列表:" << std::endl;

    // 从开始日期循环到结束日期
    for (int i = 0; i <= days; ++i)
    {
        // 计算当前日期
        std::time_t current_time = time_start + (i * seconds_per_day);
        std::tm    *current_tm   = std::localtime(&current_time);

        // 格式化为字符串
        char buffer[11];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", current_tm);
        outDates.push_back(buffer);
        // 打印日期（每7个日期一行）
        // std::cout << buffer;
        // if ((i + 1) % 7 == 0 || i == days)
        // {
        //     std::cout << std::endl;
        // }
        // else
        // {
        //     std::cout << ", ";
        // }
    }

    // 打印日期统计
    std::cout << std::endl
              << "总共 " << days + 1 << " 天" << std::endl;
}

/**
 * @brief 视频检索
 */
void Task::Retrieval::SearchByRecordType::handle()
{
    ::Common::PageInfo_S         stPageInfo; /* 页数据信息 */
    ::Record_NS::RetrievalCond_S stRecordCond = {};
    ::Event::RetrievalCond_S     stEventCond  = {};
    Convert::to_struct(m_taskData, stRecordCond, stPageInfo, stEventCond);

    /* 默认获取分段文件 */
    stRecordCond.nType = 1;
    std::vector<::Record_NS::FileInfo_S> infos;
    int                                  nRet = RecordFileManage::instance()->retrieval(stRecordCond, infos, stPageInfo);
    if (nRet < 0)
    {
        result(nRet);
    }
    else
    {
        std::string retrievalResult = Convert::to_string(infos, stPageInfo);
        result(retrievalResult);
    };
}

/* TS文件检索 */
void Task::Retrieval::SearchByRecordTS::handle()
{
    /* sd卡异常以及录制ts文件信息数据库不存在都返回空数据 */
    if (!(std::filesystem::exists(RECORD_DATABASE_PATH)) || (CStorageManage::instance()->get_SdCardStatus() != SD_CARD_STATUS_E::NORMAL))
    {
        std::string retrievalResult = "{\"reason\":\"record data is empty.\"}";
        result(retrievalResult, -1);
        return;
    }

    ::Event::RetrievalCond_S stEventCond = {};
    ::Common::PageInfo_S     stPageInfo;
    int                      nRet = 0;

    Convert::to_struct(m_taskData, stEventCond, stPageInfo);

    if (stEventCond.nChnIds.size() == 0)
    {
        std::string retrievalResult = "{\"reason\":\"The channel ID is empty.\"}";
        result(retrievalResult, -1);
        return;
    }

    if (stEventCond.strStartDate == "" || stEventCond.strEndDate == "")
    {
        std::string retrievalResult = "{\"reason\":\"The date is empty.\"}";
        result(retrievalResult, -1);
        return;
    }
    if (stEventCond.strStartTime == "" || stEventCond.strEndTime == "")
    {
        std::string retrievalResult = "{\"reason\":\"The time is empty.\"}";
        result(retrievalResult, -1);
        return;
    }

    if (!isValidDate(stEventCond.strStartDate) || !isValidDate(stEventCond.strEndDate))
    {
        std::string retrievalResult = "{\"reason\":\"The date format is error.\"}";
        result(retrievalResult, -1);
        return;
    }

    if (!isValidTime(stEventCond.strStartTime) || !isValidTime(stEventCond.strEndTime))
    {
        std::string retrievalResult = "{\"reason\":\"The time format is error.\"}";
        result(retrievalResult, -1);
        return;
    }

    /* 查找录像的所有日期 */
    std::vector<std::string> outDates;
    /* 根据设置的查找条件返回的ts文件信息 */
    std::vector<::Record_NS::TsFileInfo_S> TsFileInfos;
    /* 查找的所有日期的分页信息 */
    std::vector<::Common::PageInfo_S> stPageInfos;
    /* 查找的所有日期的条件信息 */
    std::vector<::Event::RetrievalCond_S> stEventConds;
    /* 用于比较确定当前应该查找哪一天的表格 */
    ::Common::PageInfo_S stComparePageInfo;
    /* ts文件的总数量 */
    int nTotalTsDataCount = 0;
    /* 根据设置的条件计算得出的总页数 */
    int nTotalTsPage = 0;
    int nIndex       = -1;

    calculateAndPrintDays(stEventCond.strStartDate, stEventCond.strEndDate, outDates);

    for (unsigned int i = 0; i < outDates.size(); i++)
    {
        stEventConds.push_back(stEventCond);

        stEventConds.at(i).strStartDate = outDates.at(i);
        stEventConds.at(i).strEndDate   = outDates.at(i);

        /* 第一天的查找的strStartTime与设置的一样，最后一天的strEndTime与设置的一样，其余时间为查找全天 */
        stEventConds.at(i).strStartTime = "00:00:00";
        stEventConds.at(i).strEndTime   = "23:59:59";
        if (i == 0)
        {
            stEventConds.at(i).strStartTime = stEventCond.strStartTime;
        }
        if (i == (outDates.size() - 1))
        {
            stEventConds.at(i).strEndTime = stEventCond.strEndTime;
        }
    }

    /* 求出要查询所有天数的表符合条件的ts文件的总数 */
    for (unsigned int i = 0; i < outDates.size(); i++)
    {
        nTotalTsDataCount += RecordFileManage::instance()->getTableDataCount(stEventConds.at(i), outDates.at(i));
    }
    stPageInfo.nDataTotal = nTotalTsDataCount;

    /* 求出要查询所有天数的表的总页数 */
    for (unsigned int i = 0; i < outDates.size(); i++)
    {
        ::Common::PageInfo_S stTmpPageInfo = stPageInfo;
        RecordFileManage::instance()->getTablePageInfo(stEventConds.at(i), stTmpPageInfo, outDates.at(i));
        stPageInfos.push_back(stTmpPageInfo);
        nTotalTsPage += stPageInfos.at(i).nPageTotal;
    }
    stPageInfo.nPageTotal = nTotalTsPage;

    for (unsigned int i = 0; i < outDates.size(); i++)
    {
        stComparePageInfo.nPageTotal += stPageInfos.at(i).nPageTotal;
        if (stPageInfo.nCurPage <= stComparePageInfo.nPageTotal)
        {
            /* 求出当前应该查找哪个表格 */
            nIndex = i;
            break;
        }
    }

    if (nIndex >= 0)
    {
        stPageInfos.at(nIndex).nCurPage  = stPageInfos.at(nIndex).nPageTotal - (stComparePageInfo.nPageTotal - stPageInfo.nCurPage);
        stPageInfos.at(nIndex).nPageSize = stPageInfo.nPageSize;

        /* 查找对应的表格对应的位置获取到数据ts信息TsFileInfos */
        nRet = RecordFileManage::instance()->searchByRecordTs(stEventConds.at(nIndex), TsFileInfos, stPageInfos.at(nIndex), outDates.at(nIndex));
    }

    if (nRet < 0)
    {
        result(std::string(), nRet);
    }
    else
    {
        for (unsigned int i = 0; i < TsFileInfos.size(); i++)
        {
            TsFileInfos.at(i).modifyTime = addSeconds(TsFileInfos.at(i).createTime, TsFileInfos.at(i).nDuration);
        }
        std::string retrievalResult = Convert::to_string(TsFileInfos, stPageInfo);
        result(retrievalResult, nRet);
    }
}

#if 0
/**
 * @brief 事件检索
 */
void Task::Event::SearchByEventType::handle()
{
    ::Common::PageInfo_S stPageInfo;
    ::Event::RetrievalCond_S stEventCond = {};
    Convert::to_struct(m_taskData, stEventCond, stPageInfo);
    // QuickEntry::instance()->deal(stEventCond, m_nActionCode, m_taskData);

    if (stEventCond.nChnIds.size() == 0)
    {
        std::string retrievalResult = "{\"reason\":\"The channel ID is empty.\"}";
        result(retrievalResult, -1);
        return;
    }

    if (stEventCond.strStartDate == "" || stEventCond.strEndDate == "")
    {
        std::string retrievalResult = "{\"reason\":\"The date is empty.\"}";
        result(retrievalResult, -1);
        return;
    }

    if (!isValidDate(stEventCond.strStartDate) || !isValidDate(stEventCond.strEndDate))
    {
        std::string retrievalResult = "{\"reason\":\"The date format is error.\"}";
        result(retrievalResult, -1);
        return;
    }
    
    /* 目标对比事件检索 */
    std::vector<::Event::Info_S> EventInfos;
    if (stEventCond.enType == ::Event::Type::TARGET_COMPARE)
    {
        stEventCond.enType = ::Event::Type::TARGET_CAPTURE;
        std::vector<::Event::Info_S> eventInfos;
        /* 根据事件类型检索，获取事件id */
        EventSearch::instance()->searchByEventType(stEventCond, eventInfos, stPageInfo);
        stEventCond.nChnIds.clear();

        
        /* 更新事件id为检索id */
        for (auto &stEventInfo : eventInfos)
        {
            stEventCond.nChnIds.push_back(stEventInfo.nId);
        }
        /* 查找出人脸数据 */
        std::vector<::Event::FaceCompareInfo_S> faceCompareInfos;
        EventManage::instance()->find(stEventCond, faceCompareInfos, stPageInfo);
        
    
        std::vector<::Event::Info_S> outEventInfos;
        for (auto &stFaceCompareInfo : faceCompareInfos)
        {
            for (auto &stEventInfo : eventInfos)
            {
                if (stFaceCompareInfo.nEventId == stEventInfo.nId)
                {
                    stEventInfo.enType = ::Event::Type::TARGET_COMPARE;
                    outEventInfos.push_back(stEventInfo);
                    break;
                }
            }
        }
        
        result(Convert::to_string(outEventInfos, stPageInfo));
        return;
    }
    
    int nRet = EventSearch::instance()->searchByEventType(stEventCond, EventInfos, stPageInfo);
    if (nRet < 0)
    {
        result(std::string(), nRet);
    }
    else
    {
        /* 联动视频查询 */
        if (EventInfos.size() <= 0)
        {
            std::string retrievalResult = Convert::to_string(EventInfos, stPageInfo);
            result(retrievalResult, nRet);
            return;
        }
        ::Event::RetrievalCond_S stCond;
        stCond.videoBingIds.reserve(EventInfos.size());
        for (auto &eventInfo : EventInfos)
        {
            stCond.videoBingIds.push_back(eventInfo.nId);
        }
        
        std::vector<::Event::Info_S> bindEventInfos;
        EventSearch::instance()->searchByEventType(stCond, bindEventInfos);
        for (auto &eventInfo : EventInfos)
        {
            for (auto &bindEventInfo : bindEventInfos)
            {
                if (eventInfo.nId == bindEventInfo.nVideoBindId)
                {
                    ::Event::BindVideo_S stBindVideo;
                    stBindVideo.nChnId = bindEventInfo.nChnId;
                    stBindVideo.strVideoPath = bindEventInfo.strVideoPath;
                    eventInfo.bindVideos.push_back(stBindVideo);
                }
                
                // 排序，根据 nChnId 升序
                std::sort(eventInfo.bindVideos.begin(), eventInfo.bindVideos.end(), 
                    [](const ::Event::BindVideo_S& a, const ::Event::BindVideo_S& b) {
                        return a.nChnId < b.nChnId;
                    }
                );
            }
         }
         std::string retrievalResult = Convert::to_string(EventInfos, stPageInfo);
         result(retrievalResult, nRet);
    }
}
#endif

/**
 * @brief 图片检索
 */
void Task::Retrieval::SearchByImageType::handle()
{
    /* sd卡异常以及图片信息数据库不存在都返回空数据 */
    if (!(std::filesystem::exists(CAPTURE_DATABASE_PATH)) || (CStorageManage::instance()->get_SdCardStatus() != SD_CARD_STATUS_E::NORMAL))
    {
        std::string retrievalResult = "{\"reason\":\"Capture data is empty or sd card is no exsit\"}";
        result(retrievalResult, -2);
        return;
    }

    ::Event::RetrievalCond_S stEventCond = {};
    ::Common::PageInfo_S     stPageInfo;
    Convert::to_struct(m_taskData, stEventCond, stPageInfo);

    if (stEventCond.nChnIds.size() == 0)
    {
        std::string retrievalResult = "{\"reason\":\"The channel ID is empty.\"}";
        result(retrievalResult, -1);
        return;
    }

    if (stEventCond.strStartDate == "" || stEventCond.strEndDate == "")
    {
        std::string retrievalResult = "{\"reason\":\"The date is empty.\"}";
        result(retrievalResult, -1);
        return;
    }

    if (!isValidDateTime(stEventCond.strStartDate) || !isValidDateTime(stEventCond.strEndDate))
    {
        std::string retrievalResult = "{\"reason\":\"The date format is error.\"}";
        result(retrievalResult, -1);
        return;
    }

    std::vector<Capture_NS::CaptureInfo_S> CaptureInfos;

    int nRet = EventSearch::instance()->searchByImageType(stEventCond, CaptureInfos, stPageInfo);
    if (nRet < 0)
    {
        result(std::string(), nRet);
    }
    else
    {
        /* bytes转换为kb进行显示 */
        for (auto &info : CaptureInfos)
        {
            info.nImageSize /= 1024;
        }
        std::string retrievalResult = Convert::to_string(CaptureInfos, stPageInfo);
        result(retrievalResult, nRet);
    }
}

static bool isRegularFile(const std::string &path)
{
    struct stat st;
    if (stat(path.c_str(), &st) != 0)
    {
        return false;
    }
    return S_ISREG(st.st_mode);
}

static bool isJpg(const std::string &name)
{
    auto pos = name.find_last_of('.');
    if (pos == std::string::npos)
    {
        return false;
    }

    std::string ext = name.substr(pos);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    return ext == ".jpg" || ext == ".jpeg";
}

static std::vector<std::string> scanJpgFiles(const std::string &dir)
{
    std::vector<std::string> files;

    DIR *pDir = opendir(dir.c_str());
    if (!pDir)
    {
        dlog_error("opendir error");
        return files;
    }

    struct dirent *pEntry;

    while ((pEntry = readdir(pDir)) != NULL)
    {
        std::string name = pEntry->d_name;

        if (name == "." || name == "..")
        {
            continue;
        }

        std::string path = dir + "/" + name;

        if (isRegularFile(path) && isJpg(name))
        {
            files.push_back(name);
        }
    }

    closedir(pDir);
    return files;
}

static int createTarAndRemove(const std::string &strSrcDir, const std::string &strTarFile)
{
    auto files = scanJpgFiles(strSrcDir);

    if (files.empty())
    {
        dlog_error("no jpg files");
        return -1;
    }

    std::string strCmd = "tar -czvf " + strTarFile + " -C " + strSrcDir + " ";

    for (auto &f : files)
    {
        strCmd += f + " ";
    }

    // dlog_info("cmd:[%s]", strCmd.c_str());

    int nRet = system(strCmd.c_str());

    if (!(WIFEXITED(nRet) && WEXITSTATUS(nRet) == 0))
    {
        dlog_error("执行[%s]失败", strCmd.c_str());
        return -1;
    }

    for (auto &f : files)
    {
        std::string path = strSrcDir + "/" + f;

        if (unlink(path.c_str()) != 0)
        {
            dlog_error("unlink error");
        }
    }

    return 0;
}

void Task::Retrieval::DownloadImageFileInfo::handle()
{
    /* sd卡异常以及图片信息数据库不存在都不允许下载 */
    if (!(std::filesystem::exists(CAPTURE_DATABASE_PATH)) || (CStorageManage::instance()->get_SdCardStatus() != SD_CARD_STATUS_E::NORMAL))
    {
        std::string retrievalResult = "{\"reason\":\"Capture data is empty.\"}";
        result(retrievalResult, -1);
        return;
    }

    std::vector<Capture_NS::CaptureInfo_S> stCaptureInfos;
    Convert::to_struct(m_taskData, stCaptureInfos);

    fs::path tmpDir = std::string(CAPTURE_PATH) + "/tmp";

    if (fs::exists(tmpDir))
    {
        // // 目录存在 -> 清空里面的文件
        // for (auto &entry : fs::directory_iterator(tmpDir))
        // {
        //     fs::remove_all(entry.path());
        // }

        // dlog_info("directory exists, cleaned [%s]", tmpDir.c_str())
    }
    else
    {
        // 不存在 -> 创建
        fs::create_directories(tmpDir);

        dlog_info("directory created [%s]", tmpDir.c_str());
    }

    std::string strCpyImgFileName;

    for (auto &stCaptureInfo : stCaptureInfos)
    {
        strCpyImgFileName += stCaptureInfo.strImagePath + " ";
    }
    std::string strCmd = "cp " + strCpyImgFileName + std::string(tmpDir);

    int nRet = system(strCmd.c_str());

    if (!(WIFEXITED(nRet) && WEXITSTATUS(nRet) == 0))
    {
        dlog_error("图片拷贝到临时文件失败");
        result(std::string(), -1);
    }

    std::string strTarFile = tmpDir.string() + std::string("/") + TimeUtils_NS::get_currentDateAndFormat("%Y%m%d") + "_" + TimeUtils_NS::get_currentTimeAndFormat("%H%M%S") + ".tgz";

    createTarAndRemove(tmpDir, strTarFile);

    std::string retrievalResult = "{\"Path\": \"" + strTarFile + "\"}";
    result(retrievalResult, nRet);
}

/**
 * @brief 根据日期检索
 */
void Task::Retrieval::SearchByDate::handle()
{
    ::Event::RetrievalCond_S stEventCond = {};
    Convert::to_struct(m_taskData, stEventCond);

    if (stEventCond.strStartDate == "" || stEventCond.strEndDate == "")
    {
        std::string retrievalResult = "{\"reason\":\"The date is empty.\"}";
        result(retrievalResult, -1);
    }

    if (!isValidDate(stEventCond.strStartDate) || !isValidDate(stEventCond.strEndDate))
    {
        std::string retrievalResult = "{\"reason\":\"The date format is error.\"}";
        result(retrievalResult, -1);
    }

    std::vector<::Event::Info_S> EventInfos;

    int nRet = EventSearch::instance()->searchByDate(stEventCond.strStartDate, stEventCond.strEndDate, EventInfos);
    if (nRet < 0)
    {
        result(std::string(), nRet);
    }
    else
    {
        std::string retrievalResult = Convert::to_string(EventInfos);
        result(retrievalResult, nRet);
    }
}

/**
 * @brief 根据录像时间检索(暂时没用)
 */
void Task::Retrieval::SearchByTime::handle()
{
    ::Event::RetrievalCond_S stEventCond = {};
    Convert::to_struct(m_taskData, stEventCond);

    if (stEventCond.strStartTime == "" || stEventCond.strEndTime == "")
    {
        std::string retrievalResult = "{\"reason\":\"The time is empty.\"}";
        result(retrievalResult, -1);
    }

    if (!isValidTime(stEventCond.strStartTime) || !isValidTime(stEventCond.strEndTime))
    {
        std::string retrievalResult = "{\"reason\":\"The time format is error.\"}";
        result(retrievalResult, -1);
    }

    std::vector<::Event::Info_S> EventInfos;
    int                          nRet = EventSearch::instance()->searchByTime(stEventCond.strStartTime, stEventCond.strEndTime, EventInfos);
    if (nRet < 0)
    {
        result(std::string(), nRet);
    }
    else
    {
        std::string retrievalResult = Convert::to_string(EventInfos);
        result(retrievalResult, nRet);
    }
}

/**
 * @brief 根据车辆信息检索
 */
void Task::Retrieval::SearchByVehicle::handle()
{
    ::Event::RetrievalCond_S stEventCond = {};
    Convert::to_struct(m_taskData, stEventCond);
    ::Event::RetrievalCond_S stEventCond1;
    stEventCond1.bEnQuickEntry = true;
    auto str                   = Convert::to_string(stEventCond1);
    // QuickEntry::instance()->deal(stEventCond, m_nActionCode, m_taskData);

    if (stEventCond.strStartDate == "" || stEventCond.strEndDate == "")
    {
        std::string retrievalResult = "{\"reason\":\"The date is empty.\"}";
        result(retrievalResult, -1);
    }

    if (!isValidDate(stEventCond.strStartDate) || !isValidDate(stEventCond.strEndDate))
    {
        std::string retrievalResult = "{\"reason\":\"The date format is error.\"}";
        result(retrievalResult, -1);
    }

    std::vector<::Event::VehicleInfo_S> VehicleInfos;
    int                                 nRet = EventSearch::instance()->searchByVehicle(stEventCond, VehicleInfos);
    if (nRet < 0)
    {
        result(std::string(), nRet);
    }
    else
    {
        std::string retrievalResult = Convert::to_string(VehicleInfos);
        result(retrievalResult, nRet);
    }
}

/**
 * @brief 新增普通事件
 */
void Task::Retrieval::AddNormalEvent::handle()
{
    ::Event::Info_S stEventInfo = {};
    Convert::to_struct(m_taskData, stEventInfo);

    int nRet = EventDatabaseManage::instance()->add(stEventInfo);
    result(std::string(), nRet);
}

/**
 * @brief 删除普通事件
 */
void Task::Retrieval::DelNormalEvent::handle()
{
    ::Event::Info_S stEventInfo = {};
    Convert::to_struct(m_taskData, stEventInfo);

    int nRet = EventDatabaseManage::instance()->del(stEventInfo);
    result(std::string(), nRet);
}
