/**
 * @FilePath     : record_define.h
 * @Author       : zhangjc (zhangjc@kfb.cn)
 * @Date         : 2024-10-29
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-01-07 14:36:44
 * @Description  : 录制配置信息
 */

#pragma once

#include <string>
#include <vector>
#include <memory>

#include "common_define.h"

namespace Record_NS
{

	/* 录制流媒体个数 */
	const int RECORD_REPLAY_NUM = 32;

	typedef enum Event
	{
		/* 普通事件 */
		GENERAL_ENVENTS = 0,
		/* 车辆事件 */
		CAR_ENVENTS = 1,
		/* 人体事件 */
		BOD_ENVENTS = 2
	} Event_E;

	typedef enum _RECORD_STATUS_E_
	{
		/* 无操作 */
		NO_OPERATION = 0,
		/* 录制操作 */
		RECORD_OPERATION = 1,
		/* 暂停操作 */
		PAUSE_OPERATION = 2,
		/* 停止操作 */
		STOP_OPERATION = 3
	} Status_E;

	/* 媒体数据类型 */
	typedef enum class MediaDataType
	{
		/* 视频 */
		VIDEO_DATA = 0,
		/* 音频 */
		AUDIO_DATA,
	} MediaDataType_E;

	typedef struct _RECORD_INFO
	{
		/* 录制通道 0：定时 1：事件*/
		int nChnId = -1;
		/* 视频录制状态 */
		int nVideoStatus = 0;
		/* 音频录制状态 */
		int nAudioStatus = 0;
		/* 录制状态  */
		int nRecordStatus = 0;
		/* 音视频文件格式 */
		int nRecordFormat = 0;
		/* 事件类型 */
		int nEventType = 0;
		/* 文件路径 */
		std::string path;
		/* 冗余录像文件路径 */
		std::string redunPath;
		/* 视频文件名称 */
		std::string strRecordName;
		/* 录制时间 */
		std::string strRecordTime;
		/* 码流类型：0 主码流 1 子码流 */
		int nStreamType = 0;
	} Info_S;

	typedef struct _VIDEO_CONFI_INFO_
	{
		/* 视频录制信息 */
		/* 宽 */
		int nVencWidth = 0;
		/* 高 */
		int nVencHeight = 0;
		/* 帧率 */
		int nFps = 0;
		/* ffmpeg-编码器ID */
		int nVideoCodeID = 0;
	} VideoConfigInfo_S;

	typedef struct _AUDIO_CONFI_INFO_
	{
		/* 音频录制信息 */
		/* 采样率 */
		int nSampleRate = 0;
		/* ffmpeg-编码器ID AV_CODEC_ID_AAC*/
		int nAudioCodeID = 0;
		/* 采样格式 AV_SAMPLE_FMT_S16*/
		int nSampleFmt = 0;
		/* 采样通道 1单通道，2是双通道*/
		int nChannel = 0;
	} AudioConfigInfo_S;

	/* 媒体信息 */
	typedef struct _RecordMediaData_
	{
		/* 数据类型 */
		MediaDataType_E enType = MediaDataType::VIDEO_DATA;
		/* 数据大小 */
		int nSize = 0;
		/* 是否为I帧 */
		bool bIFrame = 0;
		/* 媒体数据智能指针 */
		std::shared_ptr<char[]> pData = nullptr;
	} MediaData_S;

	typedef struct _FileInfo_S_
	{
		/* 文件ID */
		int nId = -1;
		/* 录制通道 */
		int nChnId = -1;
		/* 文件路径 */
		std::string path;
		/* 冗余录像文件路径 */
		std::string redunPath;
		/* 文件名 */
		std::string filename;
		/* 文件大小 */
		int nSize = 0;
		/* 文件类型 */
		int nType = 0; /* 0 普通视频文件，1 分段文件 */
		/* 是否锁定 */
		bool bLock = false;
		/* 文件状态 */
		int nStatus = 0;
		/* 文件创建时间 */
		std::string createTime;
		/* 文件修改时间 */
		std::string modifyTime;
		/* 文件时长 */
		int nDuration = 0;
		/* 分片文件用，连续序号 */
		int nIndex = 0;
	} FileInfo_S;

	typedef struct _TsFileInfo_S_
	{
		/* 文件ID */
		int nId = -1;
		/* 录制通道 */
		int nChnId = -1;
		/* 文件路径 */
		std::string path;
		/* 冗余录像文件路径 */
		std::string redunPath;
		/* 文件名 */
		std::string filename;
		/* 文件时长 */
		int nDuration = 0;
		/* 文件大小 */
		int nSize = 0;
		/* 文件创建时间 */
		std::string createTime;
		/* 文件创建时间 */
		std::string modifyTime;
		/* 分片文件用，连续序号 */
		int nIndex = 0;
		/* 录制视频类型：-1定时录制视频文件，其他：其他事件录制文件 */
		int nType = -1;
	} TsFileInfo_S;

    typedef struct _RecordDirInfo_
    {
        int nChnId = -1;                     /* 通道号 */
        int nCount = 0;                      /* ts文件总数 */
        long long nTotalSize = 0;            /* ts文件总大小 */
    }RecordDirInfo_S;

	typedef struct _VideoTime_S_
	{
		int nStartTime = 0;
		int nEndTime = 24 * 60 * 60;
	} VideoTime_S;

	typedef struct _Find_S_
	{
		/* 录制通道 */
		int nChnId = -1;
    	std::vector<int> chnIds;
		/* 录制年月 */
		std::string year;
		std::string month;
		/* 录制日期 */
		std::string date;
		/* 录制类型 */
		int nType = 0;
		std::string videoTime;
		std::string startTime;
		std::string endTime;
		std::string filename;
	} Find_S;

	typedef struct _TsFind_S_
	{
		/* 录制通道 */
		int nChnId = -1;
		/* 年月日 */
		std::string date;
		/* 时间 */
		std::string time;
		int nTimestamp;
		int nId = -1;
		/* -1 上一个相邻的 Id， 1 下一个相邻的 Id */
		int nType = 0;
		/* 需要查询ts文件下标 */
		int nIndex = -1;
	} TsFind_S;

	typedef struct RetrievalCond
	{
		std::vector<int> chnIds;  /* 通道号，支持多个通道 */
		int nType = -1;			  /* 事件类型 */
		std::string strStartDate; /* 开始日期 */
		std::string strEndDate;	  /* 结束日期 */
		int nIsLock = -1;		  /* 是否已锁定 0:未锁定, 1:锁定 */
	} RetrievalCond_S;

	typedef struct _FindResult_S_
	{
		/* 录制通道 */
		int nChnId = -1;
		/* 存在录像的日期 */
		std::vector<std::string> dates;
		/* 指定日期的录制文件名 */
		std::string filename;
		std::vector<VideoTime_S> videoTimes;
	} FindResult_S;

	typedef struct _UpdateInfo_S_
	{
		/* 文件id */
		int nId = -1;
		/* 新文件消息 */
		FileInfo_S stNewFileInfo;
	} UpdateInfo_S;

	/**
	 * @brief 星期一~日枚举
	 */
	typedef enum class DayOfWeek
	{
		Monday = 1,
		Tuesday,
		Wednesday,
		Thursday,
		Friday,
		Saturday,
		Sunday
	} DayOfWeek_E;

	/* 预录时间枚举 */
	typedef enum
	{
		RECORD_PRE_TIME_0_SEC   = 0,
		RECORD_PRE_TIME_5_SEC   = 5,
		RECORD_PRE_TIME_10_SEC  = 10,
		RECORD_PRE_TIME_15_SEC  = 15,
		RECORD_PRE_TIME_20_SEC  = 20,
		RECORD_PRE_TIME_25_SEC  = 25,
		RECORD_PRE_TIME_30_SEC  = 30,
		RECORD_PRE_TIME_UNLIMITED = 999
	} RecordPreTime_E;
	
	/* 录像延时枚举 */
	typedef enum
	{
		RECORD_DELAY_TIME_5_SEC  = 5,
		RECORD_DELAY_TIME_10_SEC = 10,
		RECORD_DELAY_TIME_1_MIN  = 60,
		RECORD_DELAY_TIME_2_MIN  = 120,
		RECORD_DELAY_TIME_5_MIN  = 300,
		RECORD_DELAY_TIME_10_MIN = 600
	} RecordDelayTime_E;

	/**
	 * @brief 录制时间段, 开始时间和结束时间
	 */
	typedef struct _RecordTime_S_
	{
		/* 类型：1定时录制，2事件录制 */
		int nType = 1;
		/* 开始时间 */
		int nStartTime = 0;
		/* 结束时间 */
		int nEndTime = 24 * 60 * 60;
	} RecordTime_S;

	/**
	 * @brief 每天录制计划
	 */
	typedef struct _DaySchedule_S_
	{
		/* 星期几  */
		DayOfWeek_E enDayOfWeek = DayOfWeek::Monday;
		/* 时间段, 最多设置8段 */
		std::vector<RecordTime_S> recordTimes;
	} DaySchedule_S;

	/**
	 * @brief 录制计划
	 */
	typedef struct _Schedule_S_
	{
		/* 是否开启录制 */
		bool bEnable;
		/* 录制计划, 周一到周天, 7天*/
		std::vector<DaySchedule_S> daySchedules;

        /* 默认构造函数 */
        _Schedule_S_() : bEnable(true)
        {
            daySchedules.clear();
        }

        void init_weekSchedule()
		{
			daySchedules.resize(7);
			for (int i = 0; i < 7; i++)
			{
				daySchedules[i].enDayOfWeek = static_cast<DayOfWeek_E>(i + 1);
				daySchedules[i].recordTimes.resize(1);
				daySchedules[i].recordTimes[0].nType = 1;
				daySchedules[i].recordTimes[0].nStartTime = 0;
				daySchedules[i].recordTimes[0].nEndTime = 24 * 60 * 60;
			}
		}

        /* 静态方法：返回一个带有默认规则的对象 */
        static _Schedule_S_ CreateWithDefaultRule()
        {
            _Schedule_S_ obj;
            obj.init_weekSchedule();
            return obj;
        }

    } Schedule_S;

	/**
	 * @brief 高级录制参数
	 */
	typedef struct AdvancedParam
	{
		/* 录制通道 */
		// int nChnId = -1;
		/* 是否录制音频 */
		// bool bRecordAudio = false;
		/* 是否冗余录像/抓图 */
		// bool bEnRedundancy = false;
		/* 是否开启循环录制 */
		bool bLoopWrite = false;
		/* 预录时间 */
		RecordPreTime_E ePreTime = RECORD_PRE_TIME_0_SEC;
		/* 录像延迟时间 */
		RecordDelayTime_E eDelayTime = RECORD_DELAY_TIME_5_SEC;
		/* 主次码流 */
		int nStreamType = 0;
		/* 保存天数 */
		// int nSaveDays = 0;

		// bool operator<(const AdvancedParam &other) const
		// {
		// 	return nChnId < other.nChnId;
		// }
	} AdvancedParam_S;

	/**
	 * @brief 日期类型
	 */
	typedef enum class DateType
	{
		Month = 0, /* 月份 */
		WEEK = 1,	  /* 星期 */
		DAY = 2,	  /* 日份 */
	} DateType_E;

	/**
	 * @brief 录制配置其他信息
	 */
	typedef struct OtherInfo
	{
		bool bLoopWrite = true; /* 录像循环录制 */
		bool bSaveSmartInfo = true; /* 智能信息，存下来后回放要显示 */
		bool bSaveAlarmInfo = true; /* 报警信息 */
		bool bSavePic = true; /* 是否保存图片，抓拍除外 */
	} OtherInfo_S;

	/**
	 * @brief 录制状态
	 */
	typedef struct RecordStatusInfo
	{
		// int nChnId = -1;
		Record_NS::Status_E enStatus;
	} RecordStatusInfo_S;
	
	/**
	 * @brief 录像下载
	 */
	typedef struct DownloadInfo
	{
		int nChnId = -1;
		std::string path;
		std::string startTime;
		std::string endTime;
	} DownloadInfo_S;
	/**
	 * @brief 下载进度
	 */
	typedef struct DownloadProgress
	{
		std::string filename;
		int nProgress = 0;
	} DownloadProgress_S;
}