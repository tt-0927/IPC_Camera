/*** 
 * @FilePath     : isp_control.h
 * @Author       : cyc
 * @Date         : 2025-06-05 10:16:33
 * @LastEditors  : cyc
 * @LastEditTime : 2026-01-29 16:21:23
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
#include "rk_aiq_user_api2_sysctl.h"
#include "mutex"
#include <rk_aiq_user_api2_acsm.h>
#include <rk_aiq_user_api2_camgroup.h>
#include <rk_aiq_user_api2_imgproc.h>
#include <rk_aiq_user_api2_isp.h>
#include <rk_aiq_user_api2_sysctl.h>

/* isp用户参数值映射到系统值 */
#define MAP_USER_TO_SYSTEM(userValue, minVal, maxVal) \
    ({ \
        unsigned int _uv = (userValue > 100) ? 100 : userValue; \
        unsigned int _mapped = minVal + (_uv * (maxVal - minVal)) / 100; \
        (_mapped < minVal) ? minVal : ((_mapped > maxVal) ? maxVal : _mapped); \
    })
/* isp系统参数值映射到用户值 */
#define MAP_SYSTEM_TO_USER(systemValue, minVal, maxVal) \
    ({ \
        unsigned int _sv = (systemValue < minVal) ? minVal : ((systemValue > maxVal) ? maxVal : systemValue); \
        unsigned int _user = ((_sv - minVal) * 100) / (maxVal - minVal); \
        (_user > 100) ? 100 : _user; \
    })

/* IQ文件路径 */
#define CAMERA_IQFILE_PATH "/etc/iqfiles"
/* 镜头IQ配置文件路径 */
#define CAMERA_GDCFILE_PATH "/oem/usr/etc/iqfiles/sc850sl_CMK-OT2115-PC1_ldc.ini"

/* 将增益0~100映射到0.5-3.9 */
#define MAP_100_TO_RANGE(x) (0.034f * (x) + 0.5f)

#ifdef DEVICE_TV_3882TI
    #define MAP_100_TO_RGGAIN(x) (0.029526f * (x - 12) + 0.5f)
    #define MAP_100_TO_BGGAIN(x) (0.03315f * (x - 10) + 0.5f)    
#endif

/* 将增益0.5-3.9 映射到0~100 */
#define MAP_RANGE_TO_100(y) ((unsigned int)(((y) - 0.5f) / 0.034f + 0.5f))

/* 亮度阈值 */
#define BRIGHT_MAX  (158)
#define BRIGHT_MIN  (92)
/* 对比度阈值 */
#define CONTRAST_MAX  (153)
#define CONTRAST_MIN  (89)
/* 饱和度阈值 */
#define SATURATION_MAX  (180)
#define SATURATION_MIN  (84)

//曝光时间配置表
static const ae_dynSetpoint_t AE_DYN_SETPOINT_TABLE[] = {
    {7, {0.0f, 0.1f, 0.5f, 1.0f, 4.0f, 8.0f, 20.0f}, {90.0f, 87.0f, 85.0f, 83.0f, 80.0f, 78.0f, 73.0f}},   // 0: 1/3
    {7, {0.0f, 0.1f, 0.5f, 1.0f, 4.0f, 8.0f, 20.0f}, {70.0f, 67.0f, 65.0f, 63.0f, 60.0f, 58.0f, 53.0f}},   // 1: 1/6
    {7, {0.0f, 0.1f, 0.5f, 1.0f, 4.0f, 8.0f, 20.0f}, {55.0f, 52.0f, 50.0f, 48.0f, 45.0f, 42.0f, 38.0f}},   // 2: 1/12
    {7, {0.0f, 0.1f, 0.5f, 1.0f, 4.0f, 8.0f, 20.0f}, {52.0f, 49.0f, 47.0f, 45.0f, 42.0f, 39.0f, 34.0f}},   // 3: 1/25
    {7, {0.0f, 0.1f, 0.5f, 1.0f, 4.0f, 8.0f, 20.0f}, {50.0f, 47.0f, 45.0f, 43.0f, 40.0f, 37.0f, 32.0f}},   // 4: 1/50
    {7, {0.0f, 0.1f, 0.5f, 1.0f, 4.0f, 8.0f, 20.0f}, {47.0f, 44.0f, 42.0f, 40.0f, 37.0f, 34.0f, 29.0f}},   // 5: 1/100
    {7, {0.0f, 0.1f, 0.5f, 1.0f, 4.0f, 8.0f, 20.0f}, {45.0f, 42.0f, 40.0f, 38.0f, 35.0f, 32.0f, 27.0f}},   // 6: 1/150
    {7, {0.0f, 0.1f, 0.5f, 1.0f, 4.0f, 8.0f, 20.0f}, {43.0f, 40.0f, 38.0f, 36.0f, 33.0f, 30.0f, 25.0f}},   // 7: 1/200
    {7, {0.0f, 0.1f, 0.5f, 1.0f, 4.0f, 8.0f, 20.0f}, {40.0f, 37.0f, 35.0f, 33.0f, 30.0f, 27.0f, 22.0f}},   // 8: 1/250
    {7, {0.0f, 0.1f, 0.5f, 1.0f, 4.0f, 8.0f, 20.0f}, {37.0f, 34.0f, 32.0f, 30.0f, 27.0f, 24.0f, 19.0f}},   // 9: 1/500
    {7, {0.0f, 0.1f, 0.5f, 1.0f, 4.0f, 8.0f, 20.0f}, {35.0f, 32.0f, 30.0f, 28.0f, 25.0f, 22.0f, 17.0f}},   // 10: 1/750
    {7, {0.0f, 0.1f, 0.5f, 1.0f, 4.0f, 8.0f, 20.0f}, {32.0f, 29.0f, 27.0f, 25.0f, 22.0f, 19.0f, 14.0f}},   // 11: 1/1000
    {7, {0.0f, 0.1f, 0.5f, 1.0f, 4.0f, 8.0f, 20.0f}, {30.0f, 27.0f, 25.0f, 23.0f, 20.0f, 17.0f, 12.0f}},   // 12: 1/2000
    {7, {0.0f, 0.1f, 0.5f, 1.0f, 4.0f, 8.0f, 20.0f}, {27.0f, 24.0f, 22.0f, 20.0f, 17.0f, 14.0f,  9.0f}},   // 13: 1/4000
    {7, {0.0f, 0.1f, 0.5f, 1.0f, 4.0f, 8.0f, 20.0f}, {25.0f, 22.0f, 20.0f, 18.0f, 15.0f, 12.0f,  7.0f}},   // 14: 1/10000
    {7, {0.0f, 0.1f, 0.5f, 1.0f, 4.0f, 8.0f, 20.0f}, {20.0f, 17.0f, 15.0f, 13.0f, 10.0f,  7.0f,  2.0f}},   // 15: 1/100000
};
#define AE_DYN_SETPOINT_NUM (sizeof(AE_DYN_SETPOINT_TABLE) / sizeof(AE_DYN_SETPOINT_TABLE[0]))

/// @brief 曝光时间映射表（秒）
static const float EXPOSURE_TIME_SEC_MAPPING[] = 
{
    1.0f/3.0f,    // One_3      = 0,  1/3秒
    1.0f/6.0f,    // One_6      = 1,  1/6秒
    1.0f/12.0f,   // One_12     = 2,  1/12秒
    1.0f/25.0f,   // One_25     = 3,  1/25秒
    1.0f/50.0f,   // One_50     = 4,  1/50秒
    1.0f/100.0f,  // One_100    = 5,  1/100秒
    1.0f/150.0f,  // One_150    = 6,  1/150秒
    1.0f/200.0f,  // One_200    = 7,  1/200秒
    1.0f/250.0f,  // One_250    = 8,  1/250秒
    1.0f/500.0f,  // One_500    = 9,  1/500秒
    1.0f/750.0f,  // One_750    = 10, 1/750秒
    1.0f/1000.0f, // One_1000   = 11, 1/1000秒
    1.0f/2000.0f, // One_2000   = 12, 1/2000秒
    1.0f/4000.0f, // One_4000   = 13, 1/4000秒
    1.0f/10000.0f,// One_10000  = 14, 1/10000秒
    1.0f/100000.0f// One_100000 = 15, 1/100000秒
};

/* 曝光时间补偿数组  */
static const int EXPOSURE_COMPENSATION_ARRAY[16] = {
    150, 145, 140, 135, 130, 125, 95, 90, 85, 80, 75, 70, 65, 60, 55, 50
};

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

    /*** 
     * @description : 初始化isp
     * @author      : cyc
     * param   : strIqfilesDir iq文件路径
     * @return       成功返回 0，失败返回负数
     */    
    XCamReturn rk_isp_init(const std::string strIqfilesDir);

    /*** 
     * @description : 去初始化isp
     * @author      : cyc
     * @return       成功返回 0，失败返回负数
     */    
    XCamReturn rk_isp_deInit();

    /*** 
     * @description : 获取rkaiq上下文
     * @author      : cyc
     * @return       {*}
     */    
    rk_aiq_sys_ctx_t* get_aiq_ctx();

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
    XCamReturn get_wdr_attr(WdrAttr_S &stWdrAttr) const;
    XCamReturn set_wdr_attr(const WdrAttr_S &stWdrAttr);
    /* @brief 强光抑制控制 */
    XCamReturn get_hls_attr(HlsAttr_S &stHlsAttr) const;
    XCamReturn set_hls_attr(const HlsAttr_S &stHlsAttr);
    /* @brief 背光区域控制 */
    XCamReturn get_backArea_attr(BackLightArea_E  &enBackLightArea) const;
    XCamReturn set_backArea_attr(const BackLightArea_E &enBackLightArea);
    

    /********* 基础参数控制 end ************ */  
    
private:

    int m_viPipe = 0;
    std::atomic<bool> m_isInitialized{false};

    /*cam_id*/
    int nCamID = 0;
    /*rkaiq 上下文指针*/
    rk_aiq_sys_ctx_t *g_aiq_ctx = NULL;
   /*WDRMode*/
    rk_aiq_working_mode_t g_WDRMode;
};


