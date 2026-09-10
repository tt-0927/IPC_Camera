/**
 * @FilePath     : capture_define.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-07-15 14:52:54
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-11-10 13:39:32
 * @Description  : 抓图定义
 */

#pragma once

#include <stdint.h>
#include <iostream>
#include <vector>

#include "event_define.h"
#include "video_define.h"

/* 抓图定义命名空间 */
namespace Capture_NS
{
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

	/**
	 * @brief 抓图计划时间段, 开始时间和结束时间
	 */
	typedef struct
	{
		// /* 类型：0定时抓图，1事件抓图 */
		// int nType = 0;
		/* 开始时间 */
		int nStartTime = 0;
		/* 结束时间 */
		int nEndTime = 24 * 60 * 60;
	} CaptureTime_S;

	/**
	 * @brief 每天抓图计划
	 */
	typedef struct
	{
		/* 星期几  */
		DayOfWeek_E enDayOfWeek = DayOfWeek::Monday;
		/* 时间段, 最多设置8段 */
		std::vector<CaptureTime_S> captureTimes;
	} DaySchedule_S;

    /* 抓图计划 */
    typedef struct CapturePlan
    {
        /* 抓图计划, 周一到周天, 7天*/
		std::vector<DaySchedule_S> vstDaySchedules;

        // 默认构造函数
        CapturePlan(){
            init_weekSchedule();
        }

        /**
         * @brief   : 初始周计划
         */
        void init_weekSchedule()
		{
			vstDaySchedules.resize(7);
			for (size_t i = 0; i < 7; i++)
			{
				vstDaySchedules[i].enDayOfWeek = static_cast<DayOfWeek_E>(i + 1);
				vstDaySchedules[i].captureTimes.resize(1);
				// vstDaySchedules[i].captureTimes[0].nType = 0;
				vstDaySchedules[i].captureTimes[0].nStartTime = 0;
				vstDaySchedules[i].captureTimes[0].nEndTime = 24 * 60 * 60;
			}
		}
    } CapturePlan_S;

    /* 图片格式 */
    typedef enum PictureFormat
    {
        JPEG = 0,
        BMP,
    } PictureFormat_E;

    /*图像质量*/
    typedef enum ImageQuality
    {
        LOW = 0, /* 低 */
        MEDIUM,  /* 中 */
        HIGH,    /* 高 */
    } ImageQuality_E;

    /* 时间单位枚举 */
    typedef enum TimeUnit
    {
        MILLISECONDS = 0, // 毫秒
        SECONDS,          // 秒
        MINUTES,          // 分钟
        HOURS,            // 小时
        DAYS              // 天
    } TimeUnit_E;

    /* 时间间隔结构体 */
    typedef struct TimeInterval
    {
        /* 时间间隔 */
        unsigned int unInterval;
        /* 时间单位 */
        TimeUnit_E enTimeUnit;

        void clear()
        {
            unInterval = 2000;
            enTimeUnit = MILLISECONDS;
        }

        TimeInterval()
        {
            clear();
        }
    } TimeInterval_S;

    /* 抓图定时/事件参数配置 */
    typedef struct CaptureConfig
    {
        /* 是否启用 */
        bool bEnable;
        /* 图片格式 */
        PictureFormat_E enPictureFormat;
        /* 分辨率 */
        Video_NS::VideoResolution_S stVideoResolution;
        /* 图片质量 */
        ImageQuality_E enImageQuality;
        /* 抓图时间间隔*/
        TimeInterval_S stTimeInterval;
        /* 抓图数量 */
        unsigned int unNumber;

        void clear()
        {
            bEnable = false;
            enPictureFormat = JPEG;
            stVideoResolution.nWidth = PIXEL_WIDTH_1920;
            stVideoResolution.nHeight = PIXEL_HEIGHT_1080;
            enImageQuality = MEDIUM;
            unNumber = 20;
        }

        CaptureConfig()
        {
            clear();
        }
    } CaptureConfig_S;

    /* 抓图参数 */
    typedef struct CaptureParam
    {
        /* 定时配置 */
        CaptureConfig_S stCaptureTimingConfig;
        /* 事件配置 */
        CaptureConfig_S stCaptureEventConfig;
    } CaptureParam_S;

    /* 图片格式 */
    typedef enum CaptureType
    {
        TIMING_CAPTURE = 0,
        EVENT_CAPTURE
    } CaptureType_E;

    typedef struct _CaptureInfo_S_
    {
        int nChnId = -1;                                /* 通道号 */
        Event::Type_E enType = Event::Type::SCREENSHOT; /* 事件类型 */
        std::string strStartTime;                       /* 录像开始时间 */
        std::string strEndTime;                         /* 录像结束时间 */
        // long long lTimestamp = 0;                    /* 时间戳 */
        std::string strImagePath;                       /* 图片截图路径 */
        int nImageSize = 0;                             /* 图片大小 */
        // 清空结构体的方法
        void clear()
        {
            // nId = -1;
            nChnId = 0;
            enType = Event::Type::SCREENSHOT;
            strStartTime.clear();
            strEndTime.clear();
            // lTimestamp = 0;
            strImagePath.clear();
            nImageSize = 0;
        }
    } CaptureInfo_S;

    typedef struct _CaptureDirInfo_
    {
        int nChnId = -1;          /* 通道号 */
        int nCount = 0;           /* 抓图总数 */
        long long nTotalSize = 0; /* 抓图总大小 */
    } CaptureDirInfo_S;

};  // namespace Capture_NS
