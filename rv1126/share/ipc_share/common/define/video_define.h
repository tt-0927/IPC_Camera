/**
 * @FilePath     : video_define.h
 * @Author       : zhouzirui
 * @Date         : 2025-03-21 10:55:08
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-20 17:30:00
 * @Description  : 视频定义
 */
#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "common_define.h"

//info /*----------------------- 宏定义 -----------------------*/
#define DEFAULTE_FRAMERATE (25)
#define DEFAULTE_FRAMERATE_FLOAT (25.0f)
#define DEFAULTE_BITRATE   (4096)
#define DEFAULTE_GOP       (50)

/* 分辨率宽大小定义 */
#define PIXEL_WIDTH_4K      (3840)
#define PIXEL_WIDTH_3K      (3200)
#define PIXEL_WIDTH_2_5K    (2880)
#define PIXEL_WIDTH_2K      (2560)
#define PIXEL_WIDTH_QHD     (2560)
#define PIXEL_WIDTH_1920    (1920)
#define PIXEL_WIDTH_1680    (1680)
#define PIXEL_WIDTH_1600    (1600)
#define PIXEL_WIDTH_1440    (1440)
#define PIXEL_WIDTH_1366    (1366)
#define PIXEL_WIDTH_1280    (1280)
#define PIXEL_WIDTH_1024    (1024)
#define PIXEL_WIDTH_960     (960)
#define PIXEL_WIDTH_800     (800)
#define PIXEL_WIDTH_720     (720)
#define PIXEL_WIDTH_704     (704)
#define PIXEL_WIDTH_640     (640)
#define PIXEL_WIDTH_352     (352)
/* 分辨率高大小定义 */
#define PIXEL_HEIGHT_4K     (2160)
#define PIXEL_HEIGHT_3K     (1800)
#define PIXEL_HEIGHT_2_5K   (1620)
#define PIXEL_HEIGHT_2K     (1440)
#define PIXEL_HEIGHT_QHD    (1600)
#define PIXEL_HEIGHT_1200   (1200)
#define PIXEL_HEIGHT_1080   (1080)
#define PIXEL_HEIGHT_1050   (1050)
#define PIXEL_HEIGHT_1024   (1024)
#define PIXEL_HEIGHT_960    (960)
#define PIXEL_HEIGHT_954    (954)
#define PIXEL_HEIGHT_900    (900)
#define PIXEL_HEIGHT_800    (800)
#define PIXEL_HEIGHT_768    (768)
#define PIXEL_HEIGHT_720    (720)
#define PIXEL_HEIGHT_640    (640)
#define PIXEL_HEIGHT_600    (600)
#define PIXEL_HEIGHT_576    (576)
#define PIXEL_HEIGHT_540    (540)
#define PIXEL_HEIGHT_480    (480)
#define PIXEL_HEIGHT_384    (384)
#define PIXEL_HEIGHT_360    (360)
#define PIXEL_HEIGHT_288    (288)
/* 帧率大小定义 */
#define FPS_60    (60)
#define FPS_50    (50)
#define FPS_30    (30)
#define FPS_25    (25)
#define FPS_59_94 (59.94)
#define FPS_29_97 (29.97)

/*视频定义命名空间*/
namespace Video_NS
{

#ifndef VIDEO_ROI_CONFIG_NUMBER
/* 视频感兴趣区域配置个数 */
#define VIDEO_ROI_CONFIG_NUMBER (4)
#endif

    /*视频类型*/
    typedef enum class _VideoType_E_
    {
        COMPOSITE_STREAM = 0, /*复合流*/
        VIDEO_STREAM,         /*视频流*/
    } VideoType_E;

    /*视频分辨率*/
    typedef struct _VideoResolution_S_
    {
        int nWidth = 1920;  /*视频宽度*/
        int nHeight = 1080; /*视频高度*/

        std::string to_string()
        {
            std::string strVideoResolution = std::to_string(nWidth) + "*" + std::to_string(nHeight);
            return strVideoResolution;
        }

        /**
         * @brief   : 从字符串解析并设置分辨率
         * @param    {string} &str：分辨率字符串 "1920*1080"
         * @return   {bool} true表示解析成功，false表示解析失败
         */
        bool parse_string(const std::string &str)
        {
            /* 查找分隔符'*' */
            size_t star_pos = str.find('*');
            if (star_pos == std::string::npos || star_pos == 0 || star_pos == str.size() - 1)
            {
                /* 格式错误：没有'*'，或'*'在开头/结尾 */
                return false;
            }

            /* 分割宽高字符串 */
            std::string width_str = str.substr(0, star_pos);
            std::string height_str = str.substr(star_pos + 1);
            /* 转换为整数 */
            int new_width = std::stoi(width_str);
            int new_height = std::stoi(height_str);

            /* 检查是否为有效数值（大于0） */
            if (new_width <= 0 || new_height <= 0)
            {
                return false;
            }

            /* 设置新值 */
            nWidth = new_width;
            nHeight = new_height;
            return true;
        }

        /*重载等于运算符，用于比较两个结构体是否相等*/
        bool operator==(const _VideoResolution_S_ &other) const
        {
            return (nWidth == other.nWidth) && (nHeight == other.nHeight);
        }

        /*重载不等于运算符，用于比较两个结构体是否不相等*/
        bool operator!=(const _VideoResolution_S_ &other) const
        {
            return !(*this == other);
        }
    } VideoResolution_S;

    /*码率类型*/
    typedef enum class _BitrateType_E_
    {
        CBR = 0, /*固定比特率*/
        ABR,     /*恒定平均目标码率码控算法*/
        VBR,     /*可变比特率*/
        AVBR,    /*自适应可变比特率*/
        QVBR,    /*基于主观图像质量的可变比特率*/
        CVBR,    /*以VBR为基础,设置了瞬时，短期与长期码率的限制*/
        FIXQP,   /*固定QP值*/
        QPMAP,   /*自由决定码控的策略*/
    } BitrateType_E;

    /*图像质量*/
    typedef enum class _ImageQuality_E_
    {
        LOWEST = 1,   /* 最低质量 */
        LOWER = 20,   /* 较低质量 */
        LOW = 40,     /* 低质量 */
        MEDIUM = 60,  /* 中质量 */
        HIGHER = 80,  /* 较高质量 */
        HIGHEST = 100 /* 最高质量 */
    } ImageQuality_E;

    /*视频编码*/
    typedef enum class _VideoCodec_E_
    {
        H264 = 0,
        H265,
        JPEG,
        MJPEG,
        SVAC3,
        MPEG4,
    } VideoCodec_E;

    /**
     * @brief   : 视频编码枚举转string
     * @param    {VideoCodec_E} enCodec：视频编码枚举
     * @return   {std::string} 视频编码 string 结果
     */
    inline std::string videoCodec_toString(VideoCodec_E enCodec)
    {
        switch (enCodec) 
        {
            case VideoCodec_E::H264: return "H.264";
            case VideoCodec_E::H265: return "H.265";
            case VideoCodec_E::JPEG: return "JPEG";
            case VideoCodec_E::MJPEG: return "MJPEG";
            case VideoCodec_E::SVAC3: return "SVAC3";
            case VideoCodec_E::MPEG4: return "MPEG4";
            default:
                return "16384";
        }
    }

    /**
     * @brief   : string转视频编码枚举
     * @param    {string&} str：视频编码 string
     * @return   {VideoCodec_E} 视频编码枚举
     */
    inline VideoCodec_E string_toVideoCodec(const std::string& str) 
    {
        if (str == "H.264") return VideoCodec_E::H264;
        if (str == "H.265") return VideoCodec_E::H265;
        if (str == "JPEG") return VideoCodec_E::JPEG;
        if (str == "MJPEG") return VideoCodec_E::MJPEG;
        if (str == "SVAC3") return VideoCodec_E::SVAC3;
        if (str == "MPEG4") return VideoCodec_E::MPEG4;
        return VideoCodec_E::H264;
    }

    /*编码复杂度*/
    typedef enum class _EncodingComplexity_E_
    {
        Baseline = 0, /*基线配置*/
        Main,         /*主配置*/
        High          /*高配置*/
    } EncodingComplexity_E;

    /* SVC 智能编码模式 */
    typedef enum _SvcMode_E_
    {
        SVC_MODE_DISABLE = 0, /* 关闭 */
        SVC_MODE_ENABLE = 1,  /* 开启 */
        SVC_MODE_AUTO = 2,    /* 自动 */
    } SvcMode_E;

    /* 帧率 */
    typedef enum _FrameRate_E_
    {
        FRAME_RATE_ALL = 0,  /* 所有帧率 */
        FRAME_RATE_1_16 = 1, /* 1/16 */
        FRAME_RATE_1_8 = 2,  /* 1/8 */
        FRAME_RATE_1_4 = 3,  /* 1/4 */
        FRAME_RATE_1_2 = 4,  /* 1/2 */
        FRAME_RATE_1 = 5,    /* 1 */
        FRAME_RATE_2 = 6,    /* 2 */
        FRAME_RATE_4 = 7,    /* 4 */
        FRAME_RATE_6 = 8,    /* 6 */
        FRAME_RATE_8 = 9,    /* 8 */
        FRAME_RATE_10 = 10,  /* 10 */
        FRAME_RATE_12 = 11,  /* 12 */
        FRAME_RATE_16 = 12,  /* 16 */
        FRAME_RATE_20 = 13,  /* 20 */
        FRAME_RATE_15 = 14,  /* 15 */
        FRAME_RATE_18 = 15,  /* 18 */
        FRAME_RATE_22 = 16,  /* 22 */
        FRAME_RATE_25 = 17,  /* 25 */
        FRAME_RATE_30 = 18,  /* 30 */
        FRAME_RATE_35 = 19,  /* 35 */
        FRAME_RATE_40 = 20,  /* 40 */
        FRAME_RATE_45 = 21,  /* 45 */
        FRAME_RATE_50 = 22,  /* 50 */
        FRAME_RATE_55 = 23,  /* 55 */
        FRAME_RATE_60 = 24,  /* 60 */
        FRAME_RATE_3 = 25,   /* 3 */
        FRAME_RATE_5 = 26,   /* 5 */
        FRAME_RATE_7 = 27,   /* 7 */
        FRAME_RATE_9 = 28,   /* 9 */
        FRAME_RATE_100 = 29, /* 100 */
        FRAME_RATE_120 = 30, /* 120 */
        FRAME_RATE_24 = 31,  /* 24 */
        FRAME_RATE_48 = 32,  /* 48 */
        FRAME_RATE_8_3 = 33, /* 8.3 */
        FRAME_RATE_13 = 34,  /* 13 */
        FRAME_RATE_14 = 35,  /* 14 */
        FRAME_RATE_17 = 36,  /* 17 */
        FRAME_RATE_19 = 37,  /* 19 */
        FRAME_RATE_21 = 38,  /* 21 */
        FRAME_RATE_23 = 39,  /* 23 */
        FRAME_RATE_26 = 40,  /* 26 */
        FRAME_RATE_27 = 41,  /* 27 */
        FRAME_RATE_28 = 42,  /* 28 */
        FRAME_RATE_29 = 43,  /* 29 */

        /* NOTE 枚举值总数不赋值，需要增加帧率枚举则往前插入 */
        FRAME_RATE_TOTAL, /* 枚举值总数 */
    } FrameRate_E;

    /* 视频配置 */
    typedef struct _VideoConfig_S_
    {
        int nId;                                   /*视频码流ID 0：主码流 1：子码流 2：jpeg*/
        VideoType_E enVideoType;                   /*视频类型*/
        VideoResolution_S stVideoResolution;       /*视频分辨率*/
        BitrateType_E enBitrateType;               /*码率类型*/
        ImageQuality_E enImageQuality;             /*图像质量*/
        FrameRate_E enFrameRate;                   /*视频帧率fps*/
        int nBitrateUpperLimit;                    /*码率上限kbps min="256" max="16384"*/
        int nAverageBitrate;                       /*平均码率kbps min="32" max="16384"*/
        VideoCodec_E enVideoCodec;                 /*视频编码*/
        bool bSmartEnable;                         /*智能编码*/
        EncodingComplexity_E enEncodingComplexity; /*编码复杂度*/
        int nIFrameInterval;                       /*I帧间隔 min="1" max="400"*/
        SvcMode_E enSvcEnable;                     /*SVC 智能编码*/
        int nBitrateSmoothing;                     /*码流平滑 min="1" max="100"*/

        /**
         * @brief   : 获取帧率
         * @note    : 对于分数帧率（如1/16），高16位用于表示分母，低16位表示分子
         * @return   {int} 帧率
         */
        int getFrameRateAsInt() const
        {
            switch (enFrameRate)
            {
                case FRAME_RATE_1: return 1;
                case FRAME_RATE_2: return 2;
                case FRAME_RATE_3: return 3;
                case FRAME_RATE_4: return 4;
                case FRAME_RATE_5: return 5;
                case FRAME_RATE_6: return 6;
                case FRAME_RATE_7: return 7;
                case FRAME_RATE_8: return 8;
                case FRAME_RATE_9: return 9;
                case FRAME_RATE_10: return 10;
                case FRAME_RATE_12: return 12;
                case FRAME_RATE_13: return 13;
                case FRAME_RATE_14: return 14;
                case FRAME_RATE_15: return 15;
                case FRAME_RATE_16: return 16;
                case FRAME_RATE_17: return 17;
                case FRAME_RATE_18: return 18;
                case FRAME_RATE_19: return 19;
                case FRAME_RATE_20: return 20;
                case FRAME_RATE_21: return 21;
                case FRAME_RATE_22: return 22;
                case FRAME_RATE_23: return 23;
                case FRAME_RATE_24: return 24;
                case FRAME_RATE_25: return 25;
                case FRAME_RATE_26: return 26;
                case FRAME_RATE_27: return 27;
                case FRAME_RATE_28: return 28;
                case FRAME_RATE_29: return 29;
                case FRAME_RATE_30: return 30;
                case FRAME_RATE_35: return 35;
                case FRAME_RATE_40: return 40;
                case FRAME_RATE_45: return 45;
                case FRAME_RATE_48: return 48;
                case FRAME_RATE_50: return 50;
                case FRAME_RATE_55: return 55;
                case FRAME_RATE_60: return 60;
                case FRAME_RATE_100: return 100;
                case FRAME_RATE_120: return 120;
                /* 对于分数帧率，高16位用于表示分母，低16位表示分子 */
                case FRAME_RATE_1_16: return 1 + (16 << 16);
                case FRAME_RATE_1_8: return 1 + (8 << 16);
                case FRAME_RATE_1_4: return 1 + (4 << 16);
                case FRAME_RATE_1_2: return 1 + (2 << 16);
                case FRAME_RATE_8_3: return 25 + (3 << 16);;
                default: 
                    return DEFAULTE_FRAMERATE;
            }
        }

        /**
         * @brief   : 获取帧率
         * @return   {float} 帧率
         */
        float getFrameRateAsFloat() const
        {
            switch (enFrameRate)
            {
                case FRAME_RATE_1: return 1.0f;
                case FRAME_RATE_2: return 2.0f;
                case FRAME_RATE_3: return 3.0f;
                case FRAME_RATE_4: return 4.0f;
                case FRAME_RATE_5: return 5.0f;
                case FRAME_RATE_6: return 6.0f;
                case FRAME_RATE_7: return 7.0f;
                case FRAME_RATE_8: return 8.0f;
                case FRAME_RATE_9: return 9.0f;
                case FRAME_RATE_10: return 10.0f;
                case FRAME_RATE_12: return 12.0f;
                case FRAME_RATE_13: return 13.0f;
                case FRAME_RATE_14: return 14.0f;
                case FRAME_RATE_15: return 15.0f;
                case FRAME_RATE_16: return 16.0f;
                case FRAME_RATE_17: return 17.0f;
                case FRAME_RATE_18: return 18.0f;
                case FRAME_RATE_19: return 19.0f;
                case FRAME_RATE_20: return 20.0f;
                case FRAME_RATE_21: return 21.0f;
                case FRAME_RATE_22: return 22.0f;
                case FRAME_RATE_23: return 23.0f;
                case FRAME_RATE_24: return 24.0f;
                case FRAME_RATE_25: return 25.0f;
                case FRAME_RATE_26: return 26.0f;
                case FRAME_RATE_27: return 27.0f;
                case FRAME_RATE_28: return 28.0f;
                case FRAME_RATE_29: return 29.0f;
                case FRAME_RATE_30: return 30.0f;
                case FRAME_RATE_35: return 35.0f;
                case FRAME_RATE_40: return 40.0f;
                case FRAME_RATE_45: return 45.0f;
                case FRAME_RATE_48: return 48.0f;
                case FRAME_RATE_50: return 50.0f;
                case FRAME_RATE_55: return 55.0f;
                case FRAME_RATE_60: return 60.0f;
                case FRAME_RATE_100: return 100.0f;
                case FRAME_RATE_120: return 120.0f;
                case FRAME_RATE_1_16: return 1/16.0f;
                case FRAME_RATE_1_8: return 1/8.0f;
                case FRAME_RATE_1_4: return 1/4.0f;
                case FRAME_RATE_1_2: return 1/2.0f;
                case FRAME_RATE_8_3: return 8/3.0f;
                default: 
                    return DEFAULTE_FRAMERATE_FLOAT;
            }
        }

        /**
         * @brief   : 根据整数值设置帧率枚举
         * @param    {int} nFrameRateValue：整数帧率值，例如 30
         * @note    此函数会尝试匹配最接近的FrameRate_E枚举值。
         *          如果找不到精确匹配，会选择一个合理的默认值。
         *          对于分数帧率（如1/16），高16位用于表示分母，低16位表示分子
         */
        void setFrameRate(int nFrameRateValue)
        {
            switch (nFrameRateValue)
            {
                case 1:   enFrameRate = FRAME_RATE_1; break;
                case 2:   enFrameRate = FRAME_RATE_2; break;
                case 3:   enFrameRate = FRAME_RATE_3; break;
                case 4:   enFrameRate = FRAME_RATE_4; break;
                case 5:   enFrameRate = FRAME_RATE_5; break;
                case 6:   enFrameRate = FRAME_RATE_6; break;
                case 7:   enFrameRate = FRAME_RATE_7; break;
                case 8:   enFrameRate = FRAME_RATE_8; break;
                case 9:   enFrameRate = FRAME_RATE_9; break;
                case 10:  enFrameRate = FRAME_RATE_10; break;
                case 12:  enFrameRate = FRAME_RATE_12; break;
                case 13:  enFrameRate = FRAME_RATE_13; break;
                case 14:  enFrameRate = FRAME_RATE_14; break;
                case 15:  enFrameRate = FRAME_RATE_15; break;
                case 16:  enFrameRate = FRAME_RATE_16; break;
                case 17:  enFrameRate = FRAME_RATE_17; break;
                case 18:  enFrameRate = FRAME_RATE_18; break;
                case 19:  enFrameRate = FRAME_RATE_19; break;
                case 20:  enFrameRate = FRAME_RATE_20; break;
                case 21:  enFrameRate = FRAME_RATE_21; break;
                case 22:  enFrameRate = FRAME_RATE_22; break;
                case 23:  enFrameRate = FRAME_RATE_23; break;
                case 24:  enFrameRate = FRAME_RATE_24; break;
                case 25:  enFrameRate = FRAME_RATE_25; break;
                case 26:  enFrameRate = FRAME_RATE_26; break;
                case 27:  enFrameRate = FRAME_RATE_27; break;
                case 28:  enFrameRate = FRAME_RATE_28; break;
                case 29:  enFrameRate = FRAME_RATE_29; break;
                case 30:  enFrameRate = FRAME_RATE_30; break;
                case 35:  enFrameRate = FRAME_RATE_35; break;
                case 40:  enFrameRate = FRAME_RATE_40; break;
                case 45:  enFrameRate = FRAME_RATE_45; break;
                case 48:  enFrameRate = FRAME_RATE_48; break;
                case 50:  enFrameRate = FRAME_RATE_50; break;
                case 55:  enFrameRate = FRAME_RATE_55; break;
                case 60:  enFrameRate = FRAME_RATE_60; break;
                case 100: enFrameRate = FRAME_RATE_100; break;
                case 120: enFrameRate = FRAME_RATE_120; break;
                /* 对于分数帧率，高16位用于表示分母，低16位表示分子 */
                case 1 + (16 << 16): enFrameRate = FRAME_RATE_1_16; break;
                case 1 + (8 << 16): enFrameRate = FRAME_RATE_1_8; break;
                case 1 + (4 << 16): enFrameRate = FRAME_RATE_1_4; break;
                case 1 + (2 << 16): enFrameRate = FRAME_RATE_1_2; break;
                case 25 + (3 << 16): enFrameRate = FRAME_RATE_8_3; break;
                default:
                    /* 如果没有匹配的帧率，设置为默认帧率 */
                    enFrameRate = FRAME_RATE_30;
                    break;
            }
        }

        _VideoConfig_S_() :
            nId(0),
            enVideoType(VideoType_E::COMPOSITE_STREAM),
            stVideoResolution(),
            enBitrateType(BitrateType_E::CBR),
            enImageQuality(ImageQuality_E::MEDIUM),
            enFrameRate(FRAME_RATE_25),
            nBitrateUpperLimit(0),
            nAverageBitrate(0),
            enVideoCodec(VideoCodec_E::H264),
            bSmartEnable(false),
            enEncodingComplexity(EncodingComplexity_E::Main),
            nIFrameInterval(0),
            enSvcEnable(SvcMode_E::SVC_MODE_DISABLE),
            nBitrateSmoothing(0)
        {
        }

        void print() const
        {
            std::cout << "视频配置:" << std::endl;
            std::cout << "视频码流ID:" << nId << std::endl;
            std::cout << "视频类型:" << static_cast<int>(enVideoType) << std::endl;
            std::cout << "视频分辨率:" << stVideoResolution.nWidth << "x" << stVideoResolution.nHeight << std::endl;
            std::cout << "码率类型:" << static_cast<int>(enBitrateType) << std::endl;
            std::cout << "图像质量:" << static_cast<int>(enImageQuality) << std::endl;
            std::cout << "视频帧率fps:" << enFrameRate << std::endl;
            std::cout << "码率上限kbps:" << nBitrateUpperLimit << std::endl;
            std::cout << "平均码率kbps:" << nAverageBitrate << std::endl;
            std::cout << "视频编码:" << static_cast<int>(enVideoCodec) << std::endl;
            std::cout << "智能编码:" << bSmartEnable << std::endl;
            std::cout << "编码复杂度:" << static_cast<int>(enEncodingComplexity) << std::endl;
            std::cout << "I帧间隔:" << nIFrameInterval << std::endl;
            std::cout << "SVC:" << enSvcEnable << std::endl;
            std::cout << "码流平滑:" << nBitrateSmoothing << std::endl;
            std::cout << std::endl;
        }

        bool operator<(const _VideoConfig_S_ &other) const
		{
			return nId < other.nId;
		}

        bool operator==(const _VideoConfig_S_& other) const
        {
            return nId == other.nId &&
                   enVideoType == other.enVideoType &&
                   stVideoResolution == other.stVideoResolution &&
                   enBitrateType == other.enBitrateType &&
                   enImageQuality == other.enImageQuality &&
                   enFrameRate == other.enFrameRate &&
                   nBitrateUpperLimit == other.nBitrateUpperLimit &&
                   nAverageBitrate == other.nAverageBitrate &&
                   enVideoCodec == other.enVideoCodec &&
                   bSmartEnable == other.bSmartEnable &&
                   enEncodingComplexity == other.enEncodingComplexity &&
                   nIFrameInterval == other.nIFrameInterval &&
                   enSvcEnable == other.enSvcEnable &&
                   nBitrateSmoothing == other.nBitrateSmoothing;
        }

    } VideoConfig_S;

    /**
     * @brief   : 根据视频配置估算视频单帧最大字节安全上限（含最大I帧）
     * @param   {const VideoConfig_S &} stVideoConfig：视频配置
     * @return  {std::size_t} 单帧最大字节上限，保底512KB
     * @note    : 变码率(VBR/AVBR/QVBR/CVBR)使用平均码率估算，其余码率类型使用
     *            码率上限估算；以1秒码量作为单帧安全上限，用于录制UDS单帧上限与
     *            RTSP队列字节预算，避免高码率下大I帧被固定上限击穿而整帧丢弃。
     *            低码率时返回512KB保底，保持原有行为。
     */
    inline std::size_t calcMaxFrameBytes(const VideoConfig_S &stVideoConfig)
    {
        /* 变码率家族使用平均码率，其余码率类型使用码率上限，保证CBR大I帧不被低估 */
        const bool bVariableBitrate = stVideoConfig.enBitrateType == BitrateType_E::VBR ||
                                      stVideoConfig.enBitrateType == BitrateType_E::AVBR ||
                                      stVideoConfig.enBitrateType == BitrateType_E::QVBR ||
                                      stVideoConfig.enBitrateType == BitrateType_E::CVBR;
        const int nBitrateKbps = bVariableBitrate ? stVideoConfig.nAverageBitrate : stVideoConfig.nBitrateUpperLimit;
        const std::size_t unOneSecondBytes = (nBitrateKbps > 0 ? static_cast<std::size_t>(nBitrateKbps) : 0U) * 1000U / 8U;
        const std::size_t unFloorBytes = 512U * 1024U;
        return unOneSecondBytes > unFloorBytes ? unOneSecondBytes : unFloorBytes;
    }

    /* 分辨率能力 */
    typedef struct _Resolution_S_
    {
        std::string strName;                          /* 分辨率名称(如:1280*720) */
        FrameRate_E enFrameRateMin = FRAME_RATE_1_16; /*最小视频帧率fps*/
        FrameRate_E enFrameRateMax = FRAME_RATE_30;   /*最大视频帧率fps*/
        unsigned int nBitRateMin = 256;               /*最小码率kbps min="256" */
        unsigned int nBitRateMax = 16384;             /*最大码率kbps max="16384"*/

        _Resolution_S_(const std::string &name = "")
            : strName(name)
        {
        }
    } Resolution_S;

    /* 每个编码格式的能力 */
    typedef struct _EncodeAbility_S_
    {
        /*视频编码*/
        std::string strVideoCodec;
        /* 是否支持调整编码复杂度 */
        int nSupportAdjustComplexity;
        /* 支持的编码复杂度 */
        std::vector<int> vEncodeComplexity;
        /* 编码复杂度有效个数(最多3种) */
        int nEncodeComplexityNum;
        /* 默认编码复杂度 */
        unsigned int nDefaultComplexity;
        /* 是否支持SVC */
        int bSupportSVC;
        /* 是否支持码流平滑 */
        int bSupportStreamSmooth;

        _EncodeAbility_S_(const std::string &codec = "H.264",
                          int supportComplexity = 0,
                          const std::vector<int> &complexity = { 1 },
                          int supportSVC = 0,
                          int supportStreamSmooth = 0)
            : strVideoCodec(codec),
              nSupportAdjustComplexity(supportComplexity),
              vEncodeComplexity(complexity),
              nEncodeComplexityNum(complexity.size()),
              nDefaultComplexity(complexity.empty() ? 1 : complexity[0]),
              bSupportSVC(supportSVC),
              bSupportStreamSmooth(supportStreamSmooth)
        {
        }

        void clear()
        {
            *this = _EncodeAbility_S_();
        }
    } EncodeAbility_S;

    /* 单个码流视频能力 */
    typedef struct _VideoCapability_S_
    {
        /* 视频码流ID 0：主码流 1：子码流 */
        int nId;
        /* 是否支持复合流(包含音频的码流) */
        int bSupportMultiStream;
        /* 支持的分辨率数组 */
        std::vector<Resolution_S> aResolution;
        /* 分辨率有效个数 */
        int nResolutionNum;
        /* 支持的编码格式能力数组 */
        std::vector<EncodeAbility_S> aEncodeAbility;
        /* 编码格式有效个数 */
        int nEncodeTypeNum;
        /* I帧间隔区间 */
        int nIFrameIntervalMin;
        int nIFrameIntervalMax;
        /* 码流平滑区间 */
        int nStreamSmoothMin;
        int nStreamSmoothMax;

        /**
         * @brief   : 通用的分辨率添加函数
         * @param    {int} width 分辨率宽度
         * @param    {int} height 分辨率高度
         */
        void addResolution(int width, int height)
        {
            VideoResolution_S stVideoResolution;
            stVideoResolution.nWidth = width;
            stVideoResolution.nHeight = height;
            aResolution.emplace_back(stVideoResolution.to_string());
        }

        /**
         * @brief   : 通用的编码能力添加函数
         * @param    {string} &codec 视频编码格式
         * @param    {int} supportComplexity 编码复杂度
         * @param    {vector<int>} &complexity 编码复杂度有效个数（最多三个）
         * @param    {int} supportSVC 是否支持SVC
         * @param    {int} supportStreamSmooth 是否支持码流平滑
         */
        void addEncodeAbility(const std::string &codec,
                              int supportComplexity = 0,
                              const std::vector<int> &complexity = { 1 },
                              int supportSVC = 0,
                              int supportStreamSmooth = 0)
        {
            aEncodeAbility.emplace_back(codec, supportComplexity, complexity, supportSVC, supportStreamSmooth);
        }

        _VideoCapability_S_() :
            nId(0),
            bSupportMultiStream(0),
            nResolutionNum(0),
            nEncodeTypeNum(0),
            nIFrameIntervalMin(1),
            nIFrameIntervalMax(400),
            nStreamSmoothMin(1),
            nStreamSmoothMax(100)
        {
            aResolution.clear();
            aEncodeAbility.clear();
        }
    } VideoCapability_S;

    /* 视频能力集 */
    typedef struct _VideoCapabilitySet_S_
    {
        VideoCapability_S stMain; /* 主码流 */
        VideoCapability_S stSub;  /* 子码流 */

        _VideoCapabilitySet_S_() :
            stMain(),
            stSub()
        {
        }
    } VideoCapabilitySet_S;

    /*H.264/H.265/SVAC3 NAL单元类型枚举*/
    typedef enum _NalType_E_
    {
        UNKNOWN_TYPE         = 0,   /* 未知类型 */
        /* -------------------------- H.264 类型（NAL unit type 1~12）-------------------------- */
        H264_TYPE_SLICE      = 1,   /* 非IDR帧（P/B帧切片）: 普通编码图像，依赖参考帧解码 */
        H264_TYPE_DPA        = 2,   /* 数据分区A: 少见，用于错误恢复场景 */
        H264_TYPE_DPB        = 3,   /* 数据分区B: 搭配A/C使用，极少使用 */
        H264_TYPE_DPC        = 4,   /* 数据分区C: 搭配A/B使用，极少使用 */
        H264_TYPE_IDR        = 5,   /* 关键帧（IDR帧）: 不依赖前帧，可作为解码起始帧 */
        H264_TYPE_SEI        = 6,   /* 补充增强信息（SEI）: 包含时间戳、HDR元数据等扩展信息 */
        H264_TYPE_SPS        = 7,   /* 序列参数集（SPS）: 描述图像宽高、帧率、色彩等全局参数 */
        H264_TYPE_PPS        = 8,   /* 图像参数集（PPS）: 描述切片层级的参数，依赖SPS存在 */
        H264_TYPE_AUD        = 9,   /* 访问单元分隔符: 标识每帧数据起始，用于流同步 */
        H264_TYPE_EOSEQ      = 10,  /* 序列结束: 通常用于码流收尾，提示SPS结束 */
        H264_TYPE_EOSTREAM   = 11,  /* 流结束: 表示整个编码序列的结束 */
        H264_TYPE_FILLER     = 12,  /* 填充数据: 占位符NAL，编码器用于码率控制 */

        /* -------------------------- H.265 类型（NAL unit type 0~63）-------------------------- */
        H265_TYPE_TRAIL_N    = 0,   /* 尾部帧（TRAIL_N）: 非参考帧，用于普通P/B图像 */
        H265_TYPE_TRAIL_R    = 1,   /* 尾部帧（TRAIL_R）: 参考帧，用于P/B图像引用 */
        H265_TYPE_RASL_N     = 8,   /* 随机访问跳过前导帧（RASL_N）: 解码器可跳过该帧 */
        H265_TYPE_RASL_R     = 9,   /* 随机访问跳过前导帧（RASL_R）: 可被参考但跳过渲染 */
        H265_TYPE_RADL_N     = 10,  /* 随机访问可解码前导帧（RADL_N）: 解码但不参与参考 */
        H265_TYPE_RADL_R     = 11,  /* 随机访问可解码前导帧（RADL_R）: 解码并可作为参考 */
        H265_TYPE_IDR_W_RADL = 19,  /* IDR帧（IDR_W_RADL）: 关键帧，允许前导帧 */
        H265_TYPE_IDR_N_LP   = 20,  /* IDR帧（IDR_N_LP）: 关键帧，不允许前导帧，最强随机访问点 */
        H265_TYPE_CRA        = 21,  /* 清洁随机访问帧（CRA）: 关键帧，但允许参考不完整 */
        H265_TYPE_VPS        = 32,  /* 视频参数集（VPS）: HEVC新增，支持多层/多视角参数配置 */
        H265_TYPE_SPS        = 33,  /* 序列参数集（SPS）: 描述视频帧大小、帧率、色彩空间等 */
        H265_TYPE_PPS        = 34,  /* 图像参数集（PPS）: 控制切片编码方式、预测/变换参数等 */
        H265_TYPE_AUD        = 35,  /* 访问单元分隔符: 标记帧起始，有助于码流解复用 */
        H265_TYPE_EOS        = 36,  /* 序列结束: 视频序列终止，提示解码器释放资源 */
        H265_TYPE_EOB        = 37,  /* 图像结束: 图像帧结束标志，通常忽略 */
        H265_TYPE_FILLER     = 38,  /* 填充数据: 占位符，用于码率控制 */
        H265_TYPE_SEI        = 39,  /* 前缀SEI（SEI）: HDR、音视频同步等元数据 */
        H265_TYPE_SEI_SUFFIX = 40,  /* 后缀SEI: 通常用于补充信息，与前缀SEI配合使用 */

        /* -------------------------- SVAC3 类型（NAL unit type 0~15）-------------------------- */
        SVAC3_TYPE_RESERVED_0              = 0,   /* 保留 */
        SVAC3_TYPE_NON_IDR_SLICE           = 1,   /* 非IDR图像的普通图像片 */
        SVAC3_TYPE_IDR_SLICE               = 2,   /* IDR图像的图像片 */
        SVAC3_TYPE_NON_IDR_SVC_SLICE       = 3,   /* 非IDR图像的SVC增强层图像片 */
        SVAC3_TYPE_IDR_SVC_SLICE           = 4,   /* IDR图像的SVC增强层图像片 */
        SVAC3_TYPE_SURVEILLANCE_EXTENSION  = 5,   /* 监控扩展数据单元 */
        SVAC3_TYPE_SUPPLEMENTAL_INFO       = 6,   /* 补充增强信息（SEI） */
        SVAC3_TYPE_SEQUENCE_PARAM_SET      = 7,   /* 序列参数集（SPS） */
        SVAC3_TYPE_PICTURE_PARAM_SET       = 8,   /* 图像参数集（PPS） */
        SVAC3_TYPE_SECURITY_PARAM_SET      = 9,   /* 安全参数集 */
        SVAC3_TYPE_AUTHENTICATION_DATA     = 10,  /* 认证数据 */
        SVAC3_TYPE_END_OF_STREAM           = 11,  /* 流结束 */
        SVAC3_TYPE_RESERVED_12             = 12,  /* 保留 */
        SVAC3_TYPE_RESERVED_13             = 13,  /* 本标准等效使用 */
        SVAC3_TYPE_RESERVED_14             = 14,  /* 保留 */
        SVAC3_TYPE_SVC_PIC_PARAM_SET       = 15,  /* SVC增强层图像参数集 */
    } NalType_E;

    /* H.264 参数集 */
    typedef struct
    {
        uint8_t aSps[32]; /* SPS 数据 */
        int nSpsLength;   /* SPS 数据长度 */
        uint8_t aPps[16]; /* PPS 数据 */
        int nPpsLength;   /* PPS 数据长度 */
        uint8_t aSei[16]; /* SEI 数据 */
        int nSeiLength;   /* SEI 数据长度 */
    } H264Info_S;

    /* H.265 参数集 */
    typedef struct
    {
        uint8_t aVps[64];  /* VPS 数据 (HEVC 新增) */
        int nVpsLength;    /* VPS 数据长度 */
        uint8_t aSps[128]; /* SPS 数据 (HEVC 的 SPS 比 H.264 更大) */
        int nSpsLength;    /* SPS 数据长度 */
        uint8_t aPps[32];  /* PPS 数据 */
        int nPpsLength;    /* PPS 数据长度 */
        uint8_t aSei[32];  /* SEI 数据 */
        int nSeiLength;    /* SEI 数据长度 */
    } H265Info_S;

    /* 统一的视频帧结构 */
    typedef struct
    {
        VideoCodec_E enVideoCodec; /*视频编码类型*/
        NalType_E eType;           /* NAL 单元类型 (H264/H265) */
        // union {
        //     H264Info_S stH264Info;  /* H.264 数据 */
        //     H265Info_S stH265Info;  /* H.265 数据 */
        // };
        int nLen;        /* 视频数据长度 */
        uint8_t pData[]; /* 柔性数组，视频数据紧跟在结构体后面 */
        // uint8_t *pData; /* 视频数据指针 */
    } VideoFrame_S;

    /*
     * 共享媒体帧：引用计数数据 buffer。
     * 上游（VENC 取流线程）只拷贝一次，RTSP/RTMP/录制等多个下游队列
     * 持有同一份数据的 shared_ptr 引用，避免每消费者各拷贝一份。
     * 注意：共享的是拷贝副本，不是 VENC 编码器原始 buffer（release_stream 仍需尽快调用）。
     */
    typedef struct
    {
        std::shared_ptr<uint8_t[]> pData; /* 共享数据 buffer */
        int nLen = 0;                     /* 数据长度 */
    } SharedMediaFrame_S;

    /* 视频感兴趣区域结构 */
    typedef struct _VideoRoi_
    {
        bool bEnable;              /*是否启用*/
        uint32_t u32Idx;           /* 区域编号 */
        uint32_t u32Level;         /* 提示等级 [1,6] 默认：3 */
        std::string strRegionName; /* 区域名称 */
        Common::Rect_S stRect;     /* 区域矩形坐标 */

        void print() const
        {
            std::cout << "视频ROI区域信息:" << std::endl;
            std::cout << "  启用状态: " << (bEnable ? "已启用" : "未启用") << std::endl;
            std::cout << "  区域编号: " << u32Idx << std::endl;
            std::cout << "  提示等级: " << u32Level << std::endl;
            std::cout << "  区域名称: " << (strRegionName.empty() ? "未设置" : strRegionName) << std::endl;
            std::cout << "  区域坐标:" << std::endl;
            std::cout << "    X: " << stRect.nX << ", Y: " << stRect.nY << std::endl;
            std::cout << "    宽: " << stRect.nWidth << ", 高: " << stRect.nHeight << std::endl;
        }

        _VideoRoi_() : bEnable(false), u32Idx(0), u32Level(3), strRegionName(), stRect()
        {
        }

        /* 重载赋值运算符 */
        _VideoRoi_ &operator=(const _VideoRoi_ &x)
        {
            if (this != &x)
            {
                bEnable = x.bEnable;
                u32Idx = x.u32Idx;
                u32Level = x.u32Level;
                strRegionName = x.strRegionName;
                stRect = x.stRect;
            }
            return *this;
        }
    } VideoRoi_S;

    /* 视频感兴趣区域配置 */
    typedef struct _VideoRoiConfig_S_
    {
        int nId;                             /*视频码流ID 主码流：0 第二码流：1 ... */
        std::vector<VideoRoi_S> vstVideoRoi; /*视频感兴趣区域配置*/

        _VideoRoiConfig_S_() : nId(0)
        {
            vstVideoRoi.clear();
        }

        /* 静态方法：返回一个带有默认规则的对象 */
        static _VideoRoiConfig_S_ CreateWithDefaultRule()
        {
            _VideoRoiConfig_S_ obj;
            for (size_t i = 0; i < VIDEO_ROI_CONFIG_NUMBER; ++i)
            {
                VideoRoi_S stVideoRoi;
                stVideoRoi.u32Idx = i;
                obj.vstVideoRoi.push_back(stVideoRoi);
            }
            return obj;
        }

        bool operator<(const _VideoRoiConfig_S_ &other) const
		{
			return nId < other.nId;
		}

        void print() const
        {
            std::cout << "视频感兴趣区域配置信息" << std::endl;
            std::cout << "码流号: " << nId << " ("
                      << (nId == 0 ? "主码流" : "第" + std::to_string(nId + 1) + "码流")
                      << ")" << std::endl;
            std::cout << "ROI区域数量: " << vstVideoRoi.size() << std::endl;
            std::cout << std::endl;

            for (size_t i = 0; i < vstVideoRoi.size(); i++)
            {
                std::cout << "--- ROI区域 [" << i << "] ---" << std::endl;
                vstVideoRoi[i].print();
                std::cout << std::endl;
            }
        }
    } VideoRoiConfig_S;

    /* 区域裁剪 */
    typedef struct _AreaCrop_S_
    {
        /* 是否启用 */
        bool bEnable;
        /* 视频码流ID 0：主码流 1：子码流 */
        int nId;
        /* 裁剪分辨率名称(如:1280*720) */
        VideoResolution_S stResolution;
        /* 裁剪区域 */
        Common::Rect_S stRect;
        _AreaCrop_S_() : bEnable(false), nId(-1), stResolution(), stRect()
        {
        }

        bool operator<(const _AreaCrop_S_ &other) const
		{
			return nId < other.nId;
		}
    } AreaCrop_S;

}; // namespace Video_NS
