/**
 * @FilePath     : isp_control.h
 * @Author       : cyc
 * @Date         : 2025-06-05 10:16:33
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-27 20:01:17
 * @Description  : isp参数控制模块
 */

#pragma once
#include "Singleton.h"
#include "isp_dayNight.h"
#include "isp_tuning_profile.h"
#include "isp_define.h"
#include <atomic>

class CIspControl : public CSingleton<CIspControl>
{

public:
    /**
     * @brief   : 构造 ISP 控制门面。
     * @return   {void}
     */
    CIspControl();
    /**
     * @brief   : 销毁 ISP 控制门面。
     * @return   {void}
     */
    ~CIspControl();
    friend class CSingleton<CIspControl>;

    /**
     * @brief   : 初始化 ISP 参数控制模块
     * @return   {int} OK：成功，非OK：失败
     */
    int init();

    /**
     * @brief   : 去初始化 ISP 参数控制模块
     * @return   {int} OK：成功，非OK：失败
     */
    int deinit();

    /********* 高级功能控制  ************ */

    /**
     * @brief   : 读取当前图像基础参数。
     * @param    {ISP::ImageParam_S&} stImage：参数输出
     * @return   {int} OK：成功，非OK：读取失败
     */
    int get_imageParam_attr(ISP::ImageParam_S &stImage) const;
    /**
     * @brief   : 下发图像基础参数。
     * @param    {const ISP::ImageParam_S&} stImage：已校验参数
     * @return   {int} OK：成功，非OK：设置失败
     */
    int set_imageParam_attr(const ISP::ImageParam_S &stImage);
    /**
     * @brief   : 读取当前曝光参数。
     * @param    {ISP::ExposureAttr_S&} stExpAttr：参数输出
     * @return   {int} OK：成功，非OK：读取失败
     */
    int get_exposure_attr(ISP::ExposureAttr_S &stExpAttr) const;
    /**
     * @brief   : 下发曝光及防横纹参数。
     * @param    {const ISP::ExposureAttr_S&} stExpAttr：已校验参数
     * @return   {int} OK：成功，非OK：设置失败
     */
    int set_exposure_attr(const ISP::ExposureAttr_S &stExpAttr);
    /**
     * @brief   : 读取当前白平衡参数。
     * @param    {ISP::AwbAttr_S&} stAwbInfo：参数输出
     * @return   {int} OK：成功，非OK：读取失败
     */
    int get_awb_attr(ISP::AwbAttr_S &stAwbInfo) const;
    /**
     * @brief   : 下发白平衡模式和增益。
     * @param    {const ISP::AwbAttr_S&} stAwbInfo：已校验参数
     * @return   {int} OK：成功，非OK：设置失败
     */
    int set_awb_attr(const ISP::AwbAttr_S &stAwbInfo);
    /**
     * @brief   : 读取当前降噪参数。
     * @param    {ISP::DnrAttr_S&} stDnrInfo：参数输出
     * @return   {int} OK：成功，非OK：读取失败
     */
    int get_nr_attr(ISP::DnrAttr_S &stDnrInfo) const;
    /**
     * @brief   : 下发普通或高级降噪参数。
     * @param    {const ISP::DnrAttr_S&} stDnrInfo：已校验参数
     * @return   {int} OK：成功，非OK：设置失败
     */
    int set_nr_attr(const ISP::DnrAttr_S &stDnrInfo);
    /**
     * @brief   : 读取背光、宽动态及强光抑制参数。
     * @param    {ISP::BackLightArrt_S&} stBackAttr：参数输出
     * @return   {int} OK：成功，非OK：读取失败
     */
    int get_backLight_attr(ISP::BackLightArrt_S &stBackAttr) const;
    /**
     * @brief   : 下发背光区域、宽动态和强光抑制参数。
     * @param    {const ISP::BackLightArrt_S&} stBackAttr：已校验参数
     * @return   {int} OK：成功，非OK：设置失败
     */
    int set_backLight_attr(const ISP::BackLightArrt_S &stBackAttr);
    /**
     * @brief   : 读取传感器镜像翻转状态。
     * @param    {ISP::VideoAdjust_S&} stVideoAdjust：参数输出
     * @return   {int} OK：成功，非OK：读取失败
     */
    int get_videoMirror_attr(ISP::VideoAdjust_S &stVideoAdjust) const;
    /**
     * @brief   : 设置传感器镜像翻转状态。
     * @param    {const ISP::VideoAdjust_S&} stVideoAdjust：已校验参数
     * @return   {int} OK：成功，非OK：设置失败
     */
    int set_videoMirror_attr(const ISP::VideoAdjust_S &stVideoAdjust);
    /**
     * @brief   : 读取持久化日夜参数。
     * @param    {ISP::DayNightAttr_S&} stDayNightAttr：参数输出
     * @return   {int} OK：成功，非OK：读取失败
     */
    int get_dayNight_attr(ISP::DayNightAttr_S &stDayNightAttr) const;
    /**
     * @brief   : 向自动检测器注入灵敏度。
     * @param    {const ISP::DayNightAttr_S&} stDayNightAttr：已校验参数
     * @return   {int} OK：成功
     */
    int set_dayNight_attr(const ISP::DayNightAttr_S &stDayNightAttr);

    /**
     * @brief   : 注入机型调参画像
     * @param    {const Hi3516TuningProfile_S *} pProfile：调参画像，非拥有指针
     * @return   {int} OK：成功，ERR_PARAM_NULL：参数为空
     * @note    : 应在日夜控制器启动和首次参数下发前完成注入
     */
    int set_tuning_profile(const Hi3516TuningProfile_S *pProfile);

    /**
     * @brief   : 设置本次参数下发使用的明确运行场景
     * @param    {ISP::IspRuntimeScene_E} enRuntimeScene：目标内部运行场景
     * @return   {int} OK：成功，ERR_PARAM：场景非法
     */
    int set_runtime_scene_context(ISP::IspRuntimeScene_E enRuntimeScene);

    /**
     * @brief   : 根据当前白天、夜晚白光、夜晚红外运行态应用内部 Gamma 曲线
     * @return   {int} OK：成功，非OK：失败
     * @note    : Gamma 当前不暴露网页配置，仅在日夜场景切换链路内部应用
     */
    int apply_gamma_attr();

    /********* 高级功能控制 end  ************ */

private:
    /********* 基础参数控制  ************ */

    /**
     * @brief   : 写入映射后的亮度。
     * @param    {unsigned int} nBrightness：网页亮度值
     * @return   {int} OK：成功，非OK：底层失败
     */
    int set_brightness(const unsigned int nBrightness);
    /**
     * @brief   : 读取并反映射亮度。
     * @param    {unsigned int&} nBrightness：网页亮度输出
     * @return   {int} OK：成功，非OK：底层失败
     */
    int get_brightness(unsigned int &nBrightness) const;
    /**
     * @brief   : 写入饱和度。
     * @param    {unsigned int} saturation：饱和度值
     * @return   {int} OK：成功，非OK：底层失败
     */
    int set_saturation(const unsigned int saturation);
    /**
     * @brief   : 读取饱和度。
     * @param    {unsigned int&} saturation：饱和度输出
     * @return   {int} OK：成功，非OK：底层失败
     */
    int get_saturation(unsigned int &saturation) const;
    /**
     * @brief   : 写入映射后的对比度。
     * @param    {unsigned int} contrast：网页对比度值
     * @return   {int} OK：成功，非OK：底层失败
     */
    int set_contrast(const unsigned int contrast);
    /**
     * @brief   : 读取并反映射对比度。
     * @param    {unsigned int&} contrast：网页对比度输出
     * @return   {int} OK：成功，非OK：底层失败
     */
    int get_contrast(unsigned int &contrast) const;
    /**
     * @brief   : 写入按调参画像映射的锐度。
     * @param    {unsigned int} sharpness：网页锐度值
     * @return   {int} OK：成功，非OK：底层失败
     */
    int set_sharpness(const unsigned int sharpness);
    /**
     * @brief   : 读取锐度。
     * @param    {unsigned int&} sharpness：锐度输出
     * @return   {int} OK：成功，非OK：底层失败
     */
    int get_sharpness(unsigned int &sharpness) const;
    /**
     * @brief   : 读取宽动态属性。
     * @param    {ISP::WdrAttr_S&} stWdrAttr：属性输出
     * @return   {int} OK：成功，非OK：底层失败
     */
    int get_wdr_attr(ISP::WdrAttr_S &stWdrAttr) const;
    /**
     * @brief   : 设置宽动态属性。
     * @param    {const ISP::WdrAttr_S&} stWdrAttr：属性输入
     * @return   {int} OK：成功，非OK：底层失败
     */
    int set_wdr_attr(const ISP::WdrAttr_S &stWdrAttr);
    /**
     * @brief   : 读取持久化的强光抑制网页配置。
     * @param    {ISP::HlsAttr_S&} stHlsAttr：属性输出
     * @return   {int} OK：成功，非OK：读取失败
     * @note    : HLC关闭值可能与有效tolerance等级重合，开关状态不能由MPP属性反推。
     */
    int get_hls_attr(ISP::HlsAttr_S &stHlsAttr) const;
    /**
     * @brief   : 设置强光抑制属性。
     * @param    {const ISP::HlsAttr_S&} stHlsAttr：属性输入
     * @return   {int} OK：成功，非OK：底层失败
     */
    int set_hls_attr(const ISP::HlsAttr_S &stHlsAttr);

    /**
     * @brief   : 获取当前ISP调参场景
     * @return   {IspTuningScene_E} 当前调参场景
     */
    IspTuningScene_E get_tuning_scene() const;

    /********* 基础参数控制 end ************ */

private:
    /* 平台固定的VI pipe；初始化时与MPP启动配置保持一致。 */
    int m_viPipe = 0;
    /* 原子状态供上层在停止链路中判断是否允许下发参数。 */
    std::atomic<bool> m_isInitialized{ false };
    /* memory: 调参画像由isp_business_service持有，本类只保存非拥有指针。 */
    const Hi3516TuningProfile_S *m_pTuningProfile{ nullptr };
    /* 参数映射场景由reconciler在重放前明确写入，不依赖detector的成功态同步时机。 */
    std::atomic<IspTuningScene_E> m_enTuningScene{ IspTuningScene_E::DAY };
};
