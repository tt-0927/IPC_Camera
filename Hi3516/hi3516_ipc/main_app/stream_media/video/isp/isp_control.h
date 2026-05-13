/**
 * @FilePath     : isp_control.h
 * @Author       : cyc
 * @Date         : 2025-06-05 10:16:33
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-05-07 11:34:29
 * @Description  : isp参数控制模块
 */

#pragma once
#include <iostream>
#include <mutex>
#include <thread>
#include <atomic>
#include "isp_define.h"
#include "Singleton.h"
#include "isp_dayNight.h"
#include "isp_light.h"

#define USER_TO_AWB_API(user_input) \
    ((user_input) < 0 ? 0 : \
     (user_input) > 100 ? 1000 : \
     (user_input) * 10)


/* 亮度最小值 */
#define  BRIGHT_MIN_VALUE (20)
/* 亮度最大值 */
#define  BRIGHT_MAX_VALUE (60)

#if CAP_ISP_DAY_NIGHT_DIFFERENT_TUNING
/* 夜晚白光亮度最小值 */
#define  BRIGHT_NIGHT_WHITE_MIN_VALUE (24)
/* 夜晚白光亮度最大值 */
#define  BRIGHT_NIGHT_WHITE_MAX_VALUE (64)
/* 夜晚白光亮度偏移：默认网页 50 映射到系统值 53 */
#define  BRIGHT_NIGHT_WHITE_OFFSET    (23)
#endif

/* 对比度最小值 */
#define  CONTRAST_MIN_VALUE (35)
/* 对比度最大值 */
#define  CONTRAST_MAX_VALUE (100)

/* 锐度最小值 */
#define  SHARPEN_MIN_VALUE (0)
/* 锐度最大值 */
#define  SHARPEN_MAX_VALUE (32)

#if CAP_ISP_DAY_NIGHT_DIFFERENT_TUNING
/* 夜晚白光锐度最小值（当前与白天一致，预留后续差异化） */
#define  SHARPEN_NIGHT_WHITE_MIN_VALUE SHARPEN_MIN_VALUE
/* 夜晚白光锐度最大值（当前与白天一致，预留后续差异化） */
#define  SHARPEN_NIGHT_WHITE_MAX_VALUE SHARPEN_MAX_VALUE
/* 夜晚红外锐度最小值 */
#define  SHARPEN_NIGHT_IR_MIN_VALUE (0)
/* 夜晚红外锐度最大值 */
#define  SHARPEN_NIGHT_IR_MAX_VALUE (35)
#endif

#if CAP_ISP_DAY_NIGHT_DIFFERENT_TUNING
/* 白天宽动态最小值 */
#define  WDR_DAY_MIN_VALUE (0)
/* 白天宽动态最大值 */
#define  WDR_DAY_MAX_VALUE (600)
/* 夜晚白光宽动态最小值（当前与白天一致，预留后续差异化） */
#define WDR_NIGHT_WHITE_MIN_VALUE WDR_DAY_MIN_VALUE
/* 夜晚白光宽动态最大值（当前与白天一致，预留后续差异化） */
#define WDR_NIGHT_WHITE_MAX_VALUE WDR_DAY_MAX_VALUE
/* 夜晚红外宽动态最小值 */
#define WDR_NIGHT_IR_MIN_VALUE (0)
/* 夜晚红外宽动态最大值 */
#define WDR_NIGHT_IR_MAX_VALUE (800)
#else
/* 未开启日夜差异化调参时，白天黑夜统一使用白天参数逻辑（TV-3852T* 系列） */
#define WDR_DAY_MIN_VALUE (0)
#define WDR_DAY_MAX_VALUE (600)
#endif

/* 背光默认权重 */
#define BACKLIGHT_WEIGHT_DEFAULT (1)
/* 背光权重 */
#define BACKLIGHT_WEIGHT_VALUE (3)

// 旧默认值与新默认值的差值（新50 → 旧值）
#define BRIGHT_OFFSET   (19)  // 69 - 50
#define CONTRAST_OFFSET (-26) // 24 - 50
#define SATUR_OFFSET    (2)   // 52 - 50
#define SHARPEN_OFFSET  (-4)  // 46 - 50
#if CAP_ISP_DAY_NIGHT_DIFFERENT_TUNING
#define SHARPEN_NIGHT_WHITE_OFFSET SHARPEN_OFFSET // 夜晚白光当前沿用白天锐度映射，预留后续差异化
#define SHARPEN_NIGHT_IR_OFFSET    (-10)            // 默认锐度 50 在夜晚红外映射到 22（50 - 10 = 40，40% * 35 = 14）
#define WDR_DAY_OFFSET             (0)            // 白天保持当前线性映射
#define WDR_NIGHT_WHITE_OFFSET     WDR_DAY_OFFSET // 夜晚白光当前沿用白天宽动态映射，预留后续差异化
#define WDR_NIGHT_IR_OFFSET        (38)           // 默认等级 12 在夜晚红外映射到 400（12 + 38 = 50）
#else
/* TV-3852T* 系列不区分日夜差异化调参，夜晚沿用白天 offset */
#define  WDR_DAY_OFFSET (0)
#endif

/* 白天曝光补偿数组 - 16级灵敏度 */
static const int EXPOSURE_COMPENSATION_DAY_ARRAY[16] = {
    60, 57, 54, 50, 47, 44, 43, 42, 40, 38, 36, 35, 34, 33, 32, 30
};

#if CAP_ISP_DAY_NIGHT_DIFFERENT_TUNING
/* 夜晚白光曝光补偿数组 - 当前与白天一致，预留后续差异化 */
static const int EXPOSURE_COMPENSATION_NIGHT_WHITE_ARRAY[16] = {
    60, 57, 54, 50, 47, 44, 43, 42, 40, 38, 36, 35, 34, 33, 32, 30
};

/* 夜晚红外曝光补偿数组 - 在白天基础上增加 10，范围 [42, 70]，默认 One_12 对应 64 */
static const int EXPOSURE_COMPENSATION_NIGHT_IR_ARRAY[16] = {
    70, 68, 64, 60, 56, 54, 53, 52, 50, 48, 46, 45, 44, 43, 42, 40
};
#endif

using namespace ISP;

class CIspControl : public CSingleton<CIspControl>
{

public:
    CIspControl();
    ~CIspControl();
    friend class CSingleton<CIspControl>;

    /*** 
     * @description : isp参数控制模块初始化
     * @author      : cyc
     * @return       {*}
     */    
    int init();

    /*** 
     * @description : isp参数控制模块去初始化
     * @author      : cyc
     * @return       {*}
     */    
    int deinit();

    /********* 高级功能控制  ************ */
 
    /* @brief 图像参数控制 */
    int get_imageParam_attr(ImageParam_S &stImage) const;  
    int set_imageParam_attr(const ImageParam_S stImage);
    /* @brief 曝光参数控制 */
    int get_exposure_attr(ExposureAttr_S &stExpAttr) const;
    int set_exposure_attr(const ExposureAttr_S &stExpAttr);
    /* @brief 白平衡参数控制 */
    int get_awb_attr(AwbAttr_S& stAwbInfo) const;
    int set_awb_attr(const AwbAttr_S& stAwbInfo);
    /* @brief 降噪参数控制 */
    int get_nr_attr(DnrAttr_S& stDnrInfo) const;
    int set_nr_attr(const DnrAttr_S& stDnrInfo);
    /* @brief 背光参数控制 */
    int get_backLight_attr(BackLightArrt_S& stBackAttr) const;
    int set_backLight_attr(const BackLightArrt_S& stBackAttr);
    /* @brief 镜头参数控制 */
    int get_videoMirror_attr(VideoAdjust_S& stVideoAdjust) const;
    int set_videoMirror_attr(const VideoAdjust_S& stVideoAdjust);
    /* @brief 日夜参数控制 */
    int get_dayNight_attr(DayNightAttr_S& stDayNightAttr) const;
    int set_dayNight_attr(const DayNightAttr_S& stDayNightAttr);
    /**
     * @brief   : 根据当前白天、夜晚白光、夜晚红外运行态应用内部 Gamma 曲线
     * @return   {int} 0：成功，非0：失败
     * @note    : Gamma 当前不暴露网页配置，仅在日夜场景切换链路内部应用
     */
    int apply_gamma_attr();

    /********* 高级功能控制 end  ************ */

private:

    /********* 基础参数控制  ************ */

    /* @brief 亮度控制 */
    int set_brightness(const unsigned int nBrightness);
    int get_brightness(unsigned int& nBrightness) const;
    /* @brief 饱和度控制 */
    int set_saturation(const unsigned int saturation);
    int get_saturation(unsigned int& saturation) const;
    /* @brief 对比度控制 */
    int set_contrast(const unsigned int contrast);
    int get_contrast(unsigned int& contrast) const;
    /* @brief 锐度控制 */
    int set_sharpness(const unsigned int sharpness);
    int get_sharpness(unsigned int& sharpness) const;
    /* @brief 宽动态控制 */
    int get_wdr_attr(WdrAttr_S &stWdrAttr) const;
    int set_wdr_attr(const WdrAttr_S &stWdrAttr);
    /* @brief 强光抑制控制 */
    int get_hls_attr(HlsAttr_S &stHlsAttr) const;
    int set_hls_attr(const HlsAttr_S &stHlsAttr);

    /********* 基础参数控制 end ************ */

private:
    int m_viPipe = 0;
    std::atomic<bool> m_isInitialized{ false };
};
