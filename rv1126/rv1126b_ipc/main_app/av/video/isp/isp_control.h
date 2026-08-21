/**
 * @FilePath     : isp_control.h
 * @Author       : cyc
 * @Date         : 2025-06-05 10:16:33
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-11 13:39:13
 * @Description  : isp参数控制模块
 */

#pragma once
#include <iostream>
#include <mutex>
#include <thread>
#include <atomic>
#include "isp_define.h"
#include "Singleton.h"
#include "rk_aiq_user_api2_sysctl.h"
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

/**
 * @brief RV1126B RK AIQ参数控制器。
 * @note 仅持有RK AIQ上下文并完成基础图像参数读写；共享ISP service负责日夜模式、
 *       场景仲裁和补光执行。上下文由init创建、deinit释放，其他适配器只允许借用。
 */
class CIspControl : public CSingleton<CIspControl>
{

public:
    /**
     * @brief   : 构造RK AIQ参数控制器
     * @return  {void}
     */
    CIspControl();
    /**
     * @brief   : 释放RK AIQ参数控制器
     * @return  {void}
     * @note    : 析构前必须由调用方完成共享ISP service和所有借用AIQ上下文的adapter清理。
     */
    ~CIspControl();
    friend class CSingleton<CIspControl>;

    /**
     * @brief   : 初始化RK AIQ并建立参数控制上下文
     * @return  {int} OK：成功，非OK：RK AIQ初始化失败
     * @note    : 成功后才能创建共享ISP service；失败时不向外暴露半初始化上下文。
     */
    int init();

    /**
     * @brief   : 释放RK AIQ参数控制上下文
     * @return  {int} OK：成功，非OK：RK AIQ释放失败
     * @note    : 调用方必须先停止共享service，避免adapter继续借用g_aiq_ctx。
     */
    int deinit();

    /**
     * @brief   : 使用指定IQ目录初始化RK AIQ
     * @param   {const std::string} strIqfilesDir：IQ文件目录
     * @return  {XCamReturn} XCAM_RETURN_NO_ERROR：成功，其他：RK AIQ错误码
     */
    XCamReturn rk_isp_init(const std::string strIqfilesDir);

    /**
     * @brief   : 停止并释放RK AIQ
     * @return  {XCamReturn} XCAM_RETURN_NO_ERROR：成功，其他：RK AIQ错误码
     * @note    : 释放失败时保留上下文和初始化状态，供上层重试，禁止制造悬空借用指针。
     */
    XCamReturn rk_isp_deInit();

    /**
     * @brief   : 获取当前RK AIQ上下文
     * @return  {rk_aiq_sys_ctx_t*} 非空：AIQ已初始化，空：AIQ未就绪
     * @note    : 返回值为非拥有借用指针，只能在CIspControl生命周期内使用。
     */
    rk_aiq_sys_ctx_t* get_aiq_ctx();

    /********* 高级功能控制  ************ */
 
    /**
     * @brief   : 读取共享图像配置对应的RK AIQ图像参数
     * @param   {ImageParam_S&} stImage：图像参数输出
     * @return  {int} OK：成功，非OK：RK AIQ读取失败
     */
    int get_imageParam_attr(ImageParam_S &stImage) const;  
    /**
     * @brief   : 将共享图像配置转换并写入RK AIQ
     * @param   {const ImageParam_S} stImage：待下发的图像参数
     * @return  {int} OK：成功，非OK：RK AIQ写入失败
     */
    int set_imageParam_attr(const ImageParam_S stImage);
    /**
     * @brief   : 读取RK AIQ曝光参数
     * @param   {ExposureAttr_S&} stExpAttr：曝光参数输出
     * @return  {int} OK：成功，非OK：RK AIQ读取失败
     */
    int get_exposure_attr(ExposureAttr_S &stExpAttr) const;
    /**
     * @brief   : 将共享曝光配置转换并写入RK AIQ
     * @param   {const ExposureAttr_S&} stExpAttr：待下发的曝光参数
     * @return  {int} OK：成功，非OK：RK AIQ写入失败
     */
    int set_exposure_attr(const ExposureAttr_S &stExpAttr);
    /**
     * @brief   : 读取RK AIQ白平衡参数
     * @param   {AwbAttr_S&} stAwbInfo：白平衡参数输出
     * @return  {int} OK：成功，非OK：RK AIQ读取失败
     */
    int get_awb_attr(AwbAttr_S& stAwbInfo) const;
    /**
     * @brief   : 将共享白平衡配置转换并写入RK AIQ
     * @param   {const AwbAttr_S&} stAwbInfo：待下发的白平衡参数
     * @return  {int} OK：成功，非OK：RK AIQ写入失败
     */
    int set_awb_attr(const AwbAttr_S& stAwbInfo);
    /**
     * @brief   : 读取RK AIQ降噪参数
     * @param   {DnrAttr_S&} stDnrInfo：降噪参数输出
     * @return  {int} OK：成功，非OK：RK AIQ读取失败
     */
    int get_nr_attr(DnrAttr_S& stDnrInfo) const;
    /**
     * @brief   : 将共享降噪配置转换并写入RK AIQ
     * @param   {const DnrAttr_S&} stDnrInfo：待下发的降噪参数
     * @return  {int} OK：成功，非OK：RK AIQ写入失败
     */
    int set_nr_attr(const DnrAttr_S& stDnrInfo);
    /**
     * @brief   : 读取RK AIQ背光参数
     * @param   {BackLightArrt_S&} stBackAttr：背光参数输出
     * @return  {int} OK：成功，非OK：RK AIQ读取失败
     */
    int get_backLight_attr(BackLightArrt_S& stBackAttr) const;
    /**
     * @brief   : 将共享背光配置转换并写入RK AIQ
     * @param   {const BackLightArrt_S&} stBackAttr：待下发的背光参数
     * @return  {int} OK：成功，非OK：RK AIQ写入失败
     */
    int set_backLight_attr(const BackLightArrt_S& stBackAttr);
    /**
     * @brief   : 读取RK AIQ镜像参数
     * @param   {VideoAdjust_S&} stVideoAdjust：镜像参数输出
     * @return  {int} OK：成功，非OK：RK AIQ读取失败
     */
    int get_videoMirror_attr(VideoAdjust_S& stVideoAdjust) const;
    /**
     * @brief   : 将共享镜像配置转换并写入RK AIQ
     * @param   {const VideoAdjust_S&} stVideoAdjust：待下发的镜像参数
     * @return  {int} OK：成功，非OK：RK AIQ写入失败
     */
    int set_videoMirror_attr(const VideoAdjust_S& stVideoAdjust);
    /**
     * @brief   : 读取日夜配置接口的兼容结果
     * @param   {DayNightAttr_S&} stDayNightAttr：日夜配置输出
     * @return  {int} OK：读取成功，非OK：配置读取失败
     * @note    : 只读取持久化配置，不读取或修改SmartIR、IQ场景和补光硬件。
     */
    int get_dayNight_attr(DayNightAttr_S& stDayNightAttr) const;
    /**
     * @brief   : 拒绝旧的日夜直控接口
     * @param   {const DayNightAttr_S&} stDayNightAttr：旧调用方提交的日夜配置
     * @return  {int} ERR_UNSUPPORT：必须通过共享ISP service提交
     * @note    : 该接口保留二进制/源码兼容，不得恢复旧setMode、过滤或PWM控制。
     */
    int set_dayNight_attr(const DayNightAttr_S& stDayNightAttr);

    /********* 高级功能控制 end  ************ */

private:

    /********* 基础参数控制  ************ */

    /**
     * @brief   : 将用户亮度映射到RK AIQ范围并写入
     * @param   {const unsigned int} nBrightness：用户亮度百分比[0,100]
     * @return  {int} OK：成功，非OK：参数或RK AIQ错误
     */
    int set_brightness(const unsigned int nBrightness);
    /**
     * @brief   : 从RK AIQ读取亮度并映射为用户百分比
     * @param   {unsigned int&} nBrightness：用户亮度输出
     * @return  {int} OK：成功，非OK：RK AIQ读取错误
     */
    int get_brightness(unsigned int& nBrightness) const;
    /**
     * @brief   : 将用户饱和度映射到RK AIQ范围并写入
     * @param   {const unsigned int} saturation：用户饱和度百分比[0,100]
     * @return  {int} OK：成功，非OK：参数或RK AIQ错误
     */
    int set_saturation(const unsigned int saturation);
    /**
     * @brief   : 从RK AIQ读取饱和度并映射为用户百分比
     * @param   {unsigned int&} saturation：用户饱和度输出
     * @return  {int} OK：成功，非OK：RK AIQ读取错误
     */
    int get_saturation(unsigned int& saturation) const;
    /**
     * @brief   : 将用户对比度映射到RK AIQ范围并写入
     * @param   {const unsigned int} contrast：用户对比度百分比[0,100]
     * @return  {int} OK：成功，非OK：参数或RK AIQ错误
     */
    int set_contrast(const unsigned int contrast);
    /**
     * @brief   : 从RK AIQ读取对比度并映射为用户百分比
     * @param   {unsigned int&} contrast：用户对比度输出
     * @return  {int} OK：成功，非OK：RK AIQ读取错误
     */
    int get_contrast(unsigned int& contrast) const;
    /**
     * @brief   : 将用户锐度映射到RK AIQ范围并写入
     * @param   {const unsigned int} sharpness：用户锐度百分比[0,100]
     * @return  {int} OK：成功，非OK：参数或RK AIQ错误
     */
    int set_sharpness(const unsigned int sharpness);
    /**
     * @brief   : 从RK AIQ读取锐度并映射为用户百分比
     * @param   {unsigned int&} sharpness：用户锐度输出
     * @return  {int} OK：成功，非OK：RK AIQ读取错误
     */
    int get_sharpness(unsigned int& sharpness) const;
    /**
     * @brief   : 读取RK AIQ宽动态参数
     * @param   {WdrAttr_S&} stWdrAttr：宽动态参数输出
     * @return  {XCamReturn} XCAM_RETURN_NO_ERROR：成功，其他：RK AIQ错误
     */
    XCamReturn get_wdr_attr(WdrAttr_S &stWdrAttr) const;
    /**
     * @brief   : 写入RK AIQ宽动态参数
     * @param   {const WdrAttr_S&} stWdrAttr：待下发的宽动态参数
     * @return  {XCamReturn} XCAM_RETURN_NO_ERROR：成功，其他：RK AIQ错误
     */
    XCamReturn set_wdr_attr(const WdrAttr_S &stWdrAttr);
    /**
     * @brief   : 读取RK AIQ强光抑制参数
     * @param   {HlsAttr_S&} stHlsAttr：强光抑制参数输出
     * @return  {XCamReturn} XCAM_RETURN_NO_ERROR：成功，其他：RK AIQ错误
     */
    XCamReturn get_hls_attr(HlsAttr_S &stHlsAttr) const;
    /**
     * @brief   : 写入RK AIQ强光抑制参数
     * @param   {const HlsAttr_S&} stHlsAttr：待下发的强光抑制参数
     * @return  {XCamReturn} XCAM_RETURN_NO_ERROR：成功，其他：RK AIQ错误
     */
    XCamReturn set_hls_attr(const HlsAttr_S &stHlsAttr);
    /**
     * @brief   : 读取RK AIQ背光测光区域
     * @param   {BackLightArea_E&} enBackLightArea：测光区域输出
     * @return  {XCamReturn} XCAM_RETURN_NO_ERROR：成功，其他：RK AIQ错误
     */
    XCamReturn get_backArea_attr(BackLightArea_E  &enBackLightArea) const;
    /**
     * @brief   : 写入RK AIQ背光测光区域
     * @param   {const BackLightArea_E&} enBackLightArea：待下发的测光区域
     * @return  {XCamReturn} XCAM_RETURN_NO_ERROR：成功，其他：RK AIQ错误
     */
    XCamReturn set_backArea_attr(const BackLightArea_E &enBackLightArea);
    

    /********* 基础参数控制 end ************ */  
    
private:

    /* RK视频管线编号；当前RV单管线固定为0。 */
    int m_viPipe = 0;
    /* info: 标记RK AIQ上下文是否已成功初始化，析构和上层回滚据此决定是否重试。 */
    std::atomic<bool> m_isInitialized{false};

    /* 物理摄像头编号，传给RK AIQ静态sensor信息查询接口。 */
    int nCamID = 0;
    /* memory: RK AIQ上下文由rk_isp_init创建、rk_isp_deInit释放，其他模块只借用。 */
    rk_aiq_sys_ctx_t *g_aiq_ctx = NULL;
    /* 当前RK AIQ宽动态工作模式，和共享ISP的WDR用户配置相互映射。 */
    rk_aiq_working_mode_t g_WDRMode;
};
