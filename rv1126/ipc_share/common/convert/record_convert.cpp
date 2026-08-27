/**
 * @FilePath     : record_convert.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-16 16:57:26
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-01-07 14:36:13
 * @Description  : 录制配置转换处理
 */

#include "record_convert.h"
#include "common_convert.h"
#include "convert.h" /* 这个要放在UserDefineConvert的后面 */

/* 转换函数 */
void Convert::deal(Json::Object *pRootJson, Record_NS::Info_S &stRecordInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "ChnId", stRecordInfo.nChnId); 
    convert.field(pRootJson, "VideoStatus", stRecordInfo.nVideoStatus);
    convert.field(pRootJson, "AudioStatus", stRecordInfo.nAudioStatus);
    convert.field(pRootJson, "RecordStatus", stRecordInfo.nRecordStatus);
    convert.field(pRootJson, "RecordFormat", stRecordInfo.nRecordFormat);
    convert.field(pRootJson, "EventType", stRecordInfo.nEventType);
    convert.field(pRootJson, "Path", stRecordInfo.path);
	convert.field(pRootJson, "RedunPath", stRecordInfo.redunPath);
    convert.field(pRootJson, "RecordName", stRecordInfo.strRecordName);
    convert.field(pRootJson, "RecordTime", stRecordInfo.strRecordTime);
    convert.field(pRootJson, "StreamType", stRecordInfo.nStreamType);
}
void Convert::deal(Json::Object* pRootJson, std::vector<Record_NS::FileInfo_S>  &infos, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.structure(pRootJson, "RecordFileInfos", infos);
}

void Convert::deal(Json::Object* pRootJson, std::vector<Record_NS::TsFileInfo_S> &stTsFileInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.structure(pRootJson, "RecordTsFileInfos", stTsFileInfo);
}

void Convert::deal(Json::Object *pRootJson, std::vector<Record_NS::Info_S> &vecRecordInfos, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, "RecordInfos", vecRecordInfos);
}

void Convert::deal(Json::Object* pRootJson, Record_NS::VideoConfigInfo_S &stVideoConfigInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "VencWidth", stVideoConfigInfo.nVencWidth);
	convert.field(pRootJson, "VencHeight", stVideoConfigInfo.nVencHeight);
	convert.field(pRootJson, "Fps", stVideoConfigInfo.nFps);
	convert.field(pRootJson, "VideoCodeID", stVideoConfigInfo.nVideoCodeID);
}

void Convert::deal(Json::Object* pRootJson, Record_NS::AudioConfigInfo_S &stAudioConfigInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "SampleRate", stAudioConfigInfo.nSampleRate);
	convert.field(pRootJson, "AudioCodeID", stAudioConfigInfo.nAudioCodeID);
	convert.field(pRootJson, "SampleFmt", stAudioConfigInfo.nSampleFmt);
	convert.field(pRootJson, "Channel", stAudioConfigInfo.nChannel);
}

void Convert::deal(Json::Object* pRootJson, Record_NS::MediaData_S &stMediaData, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "Type", (int &)stMediaData.enType);
	convert.field(pRootJson, "Size", stMediaData.nSize);
	convert.field(pRootJson, "IsIFrame", stMediaData.bIFrame);
}

void Convert::deal(Json::Object* pRootJson, Record_NS::FileInfo_S &stFileInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "ChnId", stFileInfo.nChnId);
	convert.field(pRootJson, "Path", stFileInfo.path);
	convert.field(pRootJson, "RedunPath", stFileInfo.redunPath);
	convert.field(pRootJson, "Filename", stFileInfo.filename);
	convert.field(pRootJson, "Size", stFileInfo.nSize);
	convert.field(pRootJson, "Type", stFileInfo.nType);
	convert.field(pRootJson, "IsLock", stFileInfo.bLock);
	convert.field(pRootJson, "Status", stFileInfo.nStatus);
	convert.field(pRootJson, "CreateTime", stFileInfo.createTime);
	convert.field(pRootJson, "ModifyTime", stFileInfo.modifyTime);
	convert.field(pRootJson, "Duration", stFileInfo.nDuration);
	convert.field(pRootJson, "Index", stFileInfo.nIndex);
}



void Convert::deal(Json::Object* pRootJson, Record_NS::TsFileInfo_S &stTsFileInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "ChnId", stTsFileInfo.nChnId);
	convert.field(pRootJson, "Size", stTsFileInfo.nSize);
	convert.field(pRootJson, "CreateTime", stTsFileInfo.createTime);
	convert.field(pRootJson, "ModifyTime", stTsFileInfo.modifyTime);
	convert.field(pRootJson, "Index", stTsFileInfo.nIndex);
	convert.field(pRootJson, "Duration", stTsFileInfo.nDuration);
	convert.field(pRootJson, "Path", stTsFileInfo.path);
	convert.field(pRootJson, "RedunPath", stTsFileInfo.redunPath);
	convert.field(pRootJson, "Filename", stTsFileInfo.filename);
}
void Convert::deal(Json::Object* pRootJson, Record_NS::Find_S &stFind, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "ChnId", stFind.nChnId);
	convert.field(pRootJson, "Date", stFind.date);
	convert.field(pRootJson, "Type", stFind.nType);
	convert.field(pRootJson, "Year", stFind.year);
	convert.field(pRootJson, "Month", stFind.month);
	convert.field(pRootJson, "StartTime", stFind.startTime);
	convert.field(pRootJson, "EndTime", stFind.endTime);
	convert.field(pRootJson, "Filename", stFind.filename);
}

void Convert::deal(Json::Object* pRootJson, Record_NS::TsFind_S &stFind, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "ChnId", stFind.nChnId);
	convert.field(pRootJson, "Date", stFind.date);
	convert.field(pRootJson, "Time", stFind.time);
	convert.field(pRootJson, "Timestamp", stFind.nTimestamp);
}

void Convert::deal(Json::Object* pRootJson, Record_NS::VideoTime_S &stVideoTime, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "StartTime", stVideoTime.nStartTime);
	convert.field(pRootJson, "EndTime", stVideoTime.nEndTime);
	
}

void Convert::deal(Json::Object* pRootJson, Record_NS::RetrievalCond_S& stRetrievalCond, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "ChnIds", stRetrievalCond.chnIds);
    convert.field(pRootJson, "Type", stRetrievalCond.nType);
    convert.field(pRootJson, "StartDate", stRetrievalCond.strStartDate);
    convert.field(pRootJson, "EndDate", stRetrievalCond.strEndDate);
    convert.field(pRootJson, "IsLock", stRetrievalCond.nIsLock);
}


void Convert::deal(Json::Object* pRootJson, Record_NS::FindResult_S &stFindResult, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "ChnId", stFindResult.nChnId);
	convert.field(pRootJson, "Dates", stFindResult.dates);
	if (bOutStruct)
	{
		convert.field(pRootJson, "Filename", stFindResult.filename);
		convert.structure(pRootJson, "VideoTimes", stFindResult.videoTimes);
	}
	else
	{
		if (!stFindResult.filename.empty())
		{
			convert.field(pRootJson, "Filename", stFindResult.filename);
		}
		if (!stFindResult.videoTimes.empty())
		{
			convert.structure(pRootJson, "VideoTimes", stFindResult.videoTimes);
		}
	}

}
void Convert::deal(Json::Object* pRootJson, std::vector<Record_NS::FindResult_S> &findResults, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}
	Convert::CConvert convert(bOutStruct);
	convert.structure(pRootJson, "Infos", findResults);
}

void Convert::deal(Json::Object* pRootJson, Record_NS::UpdateInfo_S &stUpdateInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "Id", stUpdateInfo.nId);
	convert.structure(pRootJson, stUpdateInfo.stNewFileInfo); 
}


void Convert::deal(Json::Object* pRootJson, Record_NS::RecordTime_S &stRecordTime, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "Type", stRecordTime.nType);
	convert.field(pRootJson, "StartTime", stRecordTime.nStartTime);
	convert.field(pRootJson, "EndTime", stRecordTime.nEndTime);
}


void Convert::deal(Json::Object* pRootJson, Record_NS::DaySchedule_S &stDaySchedule, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "DayOfWeek", (int &)stDaySchedule.enDayOfWeek);
	convert.structure(pRootJson, "RecordTimes", stDaySchedule.recordTimes);
}
void Convert::deal(Json::Object* pRootJson, std::vector<Record_NS::DaySchedule_S> &daySchedules, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.structure(pRootJson, "Recordchedules", daySchedules);
}


void Convert::deal(Json::Object* pRootJson, Record_NS::Schedule_S &stSchedule, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "IsEnable", stSchedule.bEnable);
	convert.structure(pRootJson, stSchedule.daySchedules);
}

void Convert::deal(Json::Object* pRootJson, Record_NS::AdvancedParam_S &stAdvancedParam, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	// convert.field(pRootJson, "ChnId", stAdvancedParam.nChnId);
	// convert.field(pRootJson, "IsRecordAudio", stAdvancedParam.bRecordAudio);
	// convert.field(pRootJson, "EnRedundancy", stAdvancedParam.bEnRedundancy);
	convert.field(pRootJson, "LoopWrite", stAdvancedParam.bLoopWrite);
	convert.field(pRootJson, "PreTime", (int &)stAdvancedParam.ePreTime);
	convert.field(pRootJson, "DelayTime", (int &)stAdvancedParam.eDelayTime);
	convert.field(pRootJson, "StreamType", stAdvancedParam.nStreamType);
	// convert.field(pRootJson, "SaveDays", stAdvancedParam.nSaveDays);
}

// void Convert::deal(Json::Object* pRootJson, std::set<Record_NS::AdvancedParam_S> &stAdvancedParam, bool bOutStruct)
// {
// 	if (!pRootJson)
// 	{
// 		return;
// 	}

// 	Convert::CConvert convert(bOutStruct);
// 	convert.structure(pRootJson, "RecordAdvancedParam", stAdvancedParam);
// }

#if 0
void Convert::deal(Json::Object* pRootJson, Record_NS::RecordQuotaConfig_S &stRecordQuotaConfig, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "ChnId", stRecordQuotaConfig.nChnId);
	convert.field(pRootJson, "VideoQuota", stRecordQuotaConfig.strVideoQuota);
	convert.field(pRootJson, "RecordPath", stRecordQuotaConfig.strRecordPath);
	convert.field(pRootJson, "EnQuota", stRecordQuotaConfig.bEnQuota);
}

void Convert::deal(Json::Object* pRootJson, std::set<Record_NS::RecordQuotaConfig_S> &stRecordQuotaConfig, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.structure(pRootJson, "RecordQuotaConfig", stRecordQuotaConfig);
}

void Convert::deal(Json::Object* pRootJson, Record_NS::RecordQuotaCopy_S &stInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "VideoQuota", stInfo.strVideoQuota);
	convert.field(pRootJson, "vecChannId", stInfo.vecChannId);
}

void Convert::deal(Json::Object* pRootJson, Record_NS::RecordDiskGroupConfig &stRecordDiskGroupConfig, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}
	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "GroupId", stRecordDiskGroupConfig.nGroupId);
	convert.field(pRootJson, "vecChannId", stRecordDiskGroupConfig.vecChannId);
}

void Convert::deal(Json::Object* pRootJson, std::set<Record_NS::RecordDiskGroupConfig_S> &stRecordDiskGroupConfig, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}
	Convert::CConvert convert(bOutStruct);
	convert.structure(pRootJson, "RecordDiskGroupConfig", stRecordDiskGroupConfig);
}

void Convert::deal(Json::Object* pRootJson, Record_NS::HolidayDate_S &stHolidayDate, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}
	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "Month", stHolidayDate.nMonth);
	convert.field(pRootJson, "WeekNum", stHolidayDate.nWeekNum);
	convert.field(pRootJson, "Week", stHolidayDate.nWeek);
	convert.field(pRootJson, "Day", stHolidayDate.nDay);
	convert.field(pRootJson, "Date", stHolidayDate.date);
}

void Convert::deal(Json::Object* pRootJson, Record_NS::HolidayInfo_S &stHolidayInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}
	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "IsEnable", stHolidayInfo.bEnable);
	convert.field(pRootJson, "Index", stHolidayInfo.nIndex);
	convert.field(pRootJson, "HolidayName", stHolidayInfo.name);
	convert.field(pRootJson, "DateType", (int &)stHolidayInfo.enDateType);
	convert.structure(pRootJson, "StartDate", stHolidayInfo.stStartDate);
	convert.structure(pRootJson, "EndDate", stHolidayInfo.stEndDate);
}

void Convert::deal(Json::Object* pRootJson, std::set<Record_NS::HolidayInfo_S> &holidayInfos, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}
	Convert::CConvert convert(bOutStruct);
	convert.structure(pRootJson, "HolidayInfos", holidayInfos);
}
void Convert::deal(Json::Object* pRootJson, std::vector<Record_NS::HolidayInfo_S> &holidayInfos, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}
	Convert::CConvert convert(bOutStruct);
	convert.structure(pRootJson, "HolidayInfos", holidayInfos);
}
#endif

void Convert::deal(Json::Object* pRootJson, Record_NS::OtherInfo_S &stOtherInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}
	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "IsLoopWrite", stOtherInfo.bLoopWrite);
	convert.field(pRootJson, "IsSaveSmartInfo", stOtherInfo.bSaveSmartInfo);
	convert.field(pRootJson, "IsSaveAlarmInfo", stOtherInfo.bSaveAlarmInfo);
	convert.field(pRootJson, "IsSavePic", stOtherInfo.bSavePic);
}

void Convert::deal(Json::Object* pRootJson, Record_NS::RecordStatusInfo_S &stInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}
	Convert::CConvert convert(bOutStruct);
	// convert.field(pRootJson, "ChnId", stInfo.nChnId);
	convert.field(pRootJson, "Status", (int &)stInfo.enStatus);
}

void Convert::deal(Json::Object* pRootJson, std::vector< Record_NS::RecordStatusInfo_S > &vecInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}
	Convert::CConvert convert(bOutStruct);
	convert.structure(pRootJson, "RecordStatus", vecInfo);
}

void Convert::deal(Json::Object* pRootJson, Record_NS::DownloadInfo_S &stDownloadInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}
	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "ChnId", stDownloadInfo.nChnId);
	convert.field(pRootJson, "Path", stDownloadInfo.path);
	convert.field(pRootJson, "StartTime", stDownloadInfo.startTime);
	convert.field(pRootJson, "EndTime", stDownloadInfo.endTime);
}
void Convert::deal(Json::Object* pRootJson, std::vector<Record_NS::DownloadInfo_S> &downloadInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}
	Convert::CConvert convert(bOutStruct);
	convert.structure(pRootJson, "DownloadInfos", downloadInfo);
}
void Convert::deal(Json::Object* pRootJson, Record_NS::DownloadProgress_S &stDownloadProgress, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}
	Convert::CConvert convert(bOutStruct);
	convert.field(pRootJson, "Filename", stDownloadProgress.filename);
	convert.field(pRootJson, "DownloadProgress", stDownloadProgress.nProgress);
}
void Convert::deal(Json::Object* pRootJson, std::vector<Record_NS::DownloadProgress_S> &downloadProgress, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}
	Convert::CConvert convert(bOutStruct);
	convert.structure(pRootJson, "DownloadProgressInfos", downloadProgress);
}