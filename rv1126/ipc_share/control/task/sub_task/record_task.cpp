/**
 * @FilePath     : record_task.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-23 16:00:04
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-12-23 16:25:51
 * @Description  : 录制任务
 */

#include "record_define.h"
#include "event_manage.h"

#include "record_task.h"
#include "record_convert.h"
#include "convert_interface.h"
#include "record_file_database.h"
#include "record_file_manage.h"
#include "record_configure.h"
#include "record_ctrl.h"
#include "path_define.h"
#include "find_record_file.h"
#include "task_publish.h"
#include "action_code.h"
#include "av_configure.h"
#include "storage_manage.h"
#include "time_utils.h"

#include <iostream>
#include <string>

/* 查找指定字符串前的一个字符 */
static bool isCharBeforeExtension(const std::string& filename, char targetChar) 
{
    // 查找 ".m3u8" 扩展名位置
    size_t extPos = filename.find(".m3u8");
    
    // 检查扩展名是否存在且不在起始位置
    if (extPos == std::string::npos) 
    {
        return false;
    }
    
    // 确保扩展名前至少有一个字符
    if (extPos == 0) 
    {
        return false;
    }
    
    // 获取扩展名前一个字符
    char charBeforeExt = filename[extPos - 1];
    
    return (charBeforeExt == targetChar);
}

/* 录制控制-调试用 */
void Task::Record::CtrlRecordInfo::handle()
{
    // CRecordCtrl::instance()->reset_status();
    // result(0);
}

/* 录制文件信息 */
void Task::Record::NoticeRecordFileInfo::handle()
{
    ::Record_NS::FileInfo_S stFileInfo;
    int nRet = 0;
    Convert::to_struct(m_taskData, stFileInfo);
    if(isCharBeforeExtension(stFileInfo.filename, '_'))
    {
        CRecordCtrl::instance()->set_eventM3u8Path(stFileInfo.path, stFileInfo.filename);
    }
    else 
    {
        nRet = RecordFileManage::instance()->add(stFileInfo);
    }
    result(nRet);
}

/*删除录制文件信息*/
void Task::Record::DelRecordFileInfo::handle()
{
    result(0);
}

/*设置录制文件信息*/
void Task::Record::SetRecordFileInfo::handle()
{
    result(0);
}

/* 查找录制文件 */
void Task::Record::FindRecordFileInfo::handle()
{
    /* sd卡异常以及录制ts文件信息数据库不存在都返回空数据 */
    if(!(std::filesystem::exists(RECORD_DATABASE_PATH)) || (CStorageManage::instance()->get_SdCardStatus() != SD_CARD_STATUS_E::NORMAL))
    {
        std::string retrievalResult = "{\"reason\":\"record data is empty.\"}";
        result(retrievalResult, -1);
        return;
    }
    ::Record_NS::Find_S stFind;
    Convert::to_struct(m_taskData, stFind);

    std::vector<::Record_NS::FindResult_S> infos;
    /* 查找事件视频, 事件视频与普通视频是一样的，只是事件视频带有事件时间 */
    bool bEventVideo = stFind.nType;
    if (bEventVideo)
    {
        stFind.nType = 0;
    }
    int nRet = RecordFileManage::instance()->find(stFind, infos);

    /* 从录制目录中获取存在m3u8的所有目录 */
    std::vector<std::string> vecResult = RecordFileManage::instance()->findM3u8Dates(RECORD_PATH, "normal");
    Record_NS::FindResult_S stFindResult;
    stFindResult.nChnId = 0;

    if(!infos.size())
    {
        stFindResult.dates = vecResult;
        infos.push_back(stFindResult);
    }
    else 
    {
        for(auto &Result : vecResult)
        {
            if(infos.size())
            {
                /* 摄像机为单通道 0，把所有获取到的日期加入到从数据库中查找到的结果，保证数据库未记录但实际有录制的情况也能查询到 */
                infos[0].dates.push_back(Result);
            }
        }
    }

    for (auto &stInfo : infos)
    {
        /* 排序，让相同字符串相邻 */
        std::sort(stInfo.dates.begin(), stInfo.dates.end());
        /* 删除重复日期 */
        stInfo.dates.erase(std::unique(stInfo.dates.begin(), stInfo.dates.end()), stInfo.dates.end());
    }

    result(Convert::to_string(infos), nRet);
}
/*通知ts录制文件信息*/
void Task::Record::NoticeRecordTsFileInfo::handle()
{
    ::Record_NS::TsFileInfo_S stTsFileInfo;
    Convert::to_struct(m_taskData, stTsFileInfo);
    int nRet = -1;
    if(!stTsFileInfo.filename.empty())
    {
        std::string strDate = TimeUtils_NS::get_currentDate();
        // CStorageManage::instance()->accumulateRecordSize(stTsFileInfo.path, stTsFileInfo.nSize, strDate);

        Record_NS::RecordDirInfo_S stDirInfo;
        stDirInfo.nChnId = stTsFileInfo.nChnId;
        nRet = RecordFileDatabase::instance()->get_itemInfo(stDirInfo);

        stDirInfo.nTotalSize += (long long)stTsFileInfo.nSize;
        stDirInfo.nCount++;

        if(nRet < 0)
        {
            RecordFileDatabase::instance()->add(stDirInfo);
        }
        else 
        {
            RecordFileDatabase::instance()->update(stDirInfo);
        }

        /* 转为k为单位 */
        stTsFileInfo.nSize /= 1024; 
        nRet = RecordFileManage::instance()->add(stTsFileInfo); 
    }
    
    result(nRet);
}
/*通知录制异常*/
void Task::Record::NoticeRecordException::handle()
{
    /* 处理缓存文件 */
    RecordFileManage::instance()->deal_cacheFile();
}

/* 获取人为录制信息（预览-录制） */
void Task::Record::GetHumanRecord::handle()
{
    ::Record_NS::Info_S stInfo;
    CRecordCtrl::instance()->get_humanRecord(stInfo);
    result(Convert::to_string(stInfo));
}

/* 设置人为录制信息（预览-录制） */
void Task::Record::SetHumanRecord::handle()
{
    ::Record_NS::Info_S stInfo;
    Convert::to_struct(m_taskData, stInfo);
    ::Record_NS::Info_S stOldInfo;
    CRecordCtrl::instance()->get_humanRecord(stOldInfo);
    if (stOldInfo.nRecordStatus == ::Record_NS::Status_E::RECORD_OPERATION && stInfo.nRecordStatus == ::Record_NS::Status_E::RECORD_OPERATION)
    {
        dlog_error("正在录制，不能同时开两个录制任务");
        result(-1);
        return;
    }
    int nRet = CRecordCtrl::instance()->set_humanRecord(stInfo);
    result(nRet);
}

/*下载录制文件*/
void Task::Record::DownloadRecordFile::handle()
{
    std::vector<::Record_NS::DownloadInfo_S> downloadInfos;
    Convert::to_struct(m_taskData, downloadInfos);
    
    std::vector<::Record_NS::DownloadProgress_S> downloadProgress;
    for (auto &stDownloadInfo : downloadInfos)
    {
        std::string outFilename = "D" + std::to_string(stDownloadInfo.nChnId + 1) + "_cut_" 
            + stDownloadInfo.startTime + "_" + stDownloadInfo.endTime + ".ts";
        ::Record_NS::DownloadProgress_S stDownloadProgress;
        stDownloadProgress.filename = outFilename;
        stDownloadProgress.nProgress = -1;
        downloadProgress.push_back(stDownloadProgress);
        if (stDownloadInfo.path.empty())
        {
            dlog_error("下载路径为空");
            downloadProgress.back().nProgress = -1;
            continue;
        }
        FindRecordFile fileFinder;
        std::deque<std::string> files = fileFinder.find(stDownloadInfo.nChnId, stDownloadInfo.startTime, stDownloadInfo.endTime);
        if (files.empty())
        {
            dlog_error("没有找到录像文件");
            downloadProgress.back().nProgress = -1;
            continue;
        }
        
        stDownloadInfo.path += outFilename;
        dlog_error("找到录像文件 %d", files.size());
        /* lamba表达式，使用线程执行 */
        auto download_file = [stDownloadInfo, files, stDownloadProgress]() mutable
        {
            /* 写入文件 */
            std::ofstream ofs(stDownloadInfo.path, std::ios::binary | std::ios::app);
            if (!ofs)
            {
                dlog_error("打开文件失败 %s", stDownloadInfo.path.c_str());
                stDownloadProgress.nProgress = -1;
                TaskPublish::instance()->message(AC_NOTICE_DOWNLOAD_RECORD_PROGRESS, Convert::to_string(stDownloadProgress));
                return;
            }
            for (int i = 0; i < files.size(); i++)
            {
                auto &file = files[i];
                std::ifstream ifs(file, std::ios::binary);
                if (!ifs)
                {
                    dlog_error("打开文件失败 %s", file.c_str());
                    continue;
                }
                ofs << ifs.rdbuf();
                ifs.close();
                int nProgress = (i + 1) * 100 / files.size();
                if (nProgress != stDownloadProgress.nProgress)
                {   
                    stDownloadProgress.nProgress = nProgress;
                    TaskPublish::instance()->message(AC_NOTICE_DOWNLOAD_RECORD_PROGRESS, Convert::to_string(stDownloadProgress));
                }
            }
            ofs.flush();
            ofs.close();
            if (stDownloadProgress.nProgress == -1)
            {
                TaskPublish::instance()->message(AC_NOTICE_DOWNLOAD_RECORD_PROGRESS, Convert::to_string(stDownloadProgress));
            }
            else if (stDownloadProgress.nProgress != 100)
            {
                stDownloadProgress.nProgress = 100;
                TaskPublish::instance()->message(AC_NOTICE_DOWNLOAD_RECORD_PROGRESS, Convert::to_string(stDownloadProgress));
            }
        };
        /* 线程 */
        std::thread th(download_file);
        th.detach();
        downloadProgress.back().nProgress = 0;
    }
    if (downloadProgress.size() == 0)
    {
        result(-1);
        return;
    }
    result(Convert::to_string(downloadProgress));
    
}

/*通知下载录制文件进度*/
void Task::Record::NoticeDownloadRecordProgress::handle()
{
    result(0);
}

/* 获取高级录制参数 */
void Task::Record::GetAdvancedParam::handle()
{
    ::Record_NS::AdvancedParam_S stParam;
    RecordConfigure::instance()->get_configure(stParam);
    result(Convert::to_string(stParam));
}

/* 设置高级录制参数 */
void Task::Record::SetAdvancedParam::handle()
{
    ::Record_NS::AdvancedParam_S stParam;
    Convert::to_struct(m_taskData, stParam);
    RecordConfigure::instance()->set_configure(stParam);
    CRecordCtrl::instance()->update_advancedParam();
    result(Convert::to_string(stParam));
}

/* 获取录制计划 */
void Task::Record::GetSchedule::handle()
{
    ::Record_NS::Schedule_S stSchedule;
    stSchedule.daySchedules.clear();
    RecordConfigure::instance()->get_configure(stSchedule);
    result(Convert::to_string(stSchedule));
}

/* 设置录制计划 */
void Task::Record::SetSchedule::handle()
{
    ::Record_NS::Schedule_S stSchedule;
    stSchedule.daySchedules.clear();
    Convert::to_struct(m_taskData, stSchedule);
    int nRet = RecordConfigure::instance()->set_configure(stSchedule);
    CRecordCtrl::instance()->update();
    result(nRet);
}

/*获取录制配置其他信息*/
void Task::Record::GetRecordOtherInfo::handle()
{
    ::Record_NS::OtherInfo_S stOtherInfo;
    Convert::read_file(RECORD_OTHER_CONFIG_FILE, stOtherInfo);
    result(Convert::to_string(stOtherInfo));
}

/*设置录制配置其他信息*/
void Task::Record::SetRecordOtherInfo::handle()
{
    ::Record_NS::OtherInfo_S stOtherInfo;
    Convert::to_struct(m_taskData, stOtherInfo);
    Convert::write_file(RECORD_OTHER_CONFIG_FILE, stOtherInfo);
    result(0);
}

/*获取通道的录制状态*/
void Task::Record::GetRecordStatusInfo::handle()
{
    ::Record_NS::RecordStatusInfo_S stStatus;
    ::Record_NS::Status_E enStatus = CRecordCtrl::instance()->get_recordStatus();
    stStatus.enStatus = enStatus;

    result(Convert::to_string(stStatus));
}