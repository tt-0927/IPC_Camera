/**
 * @FilePath     : isp_service_interface.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-10 15:19:22
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-22 10:20:06
 * @Description  : ISP业务服务接口，由业务仓库实现并注册到CIspManage
 */

#pragma once

#include "isp_define.h"
#include "isp_config_value.h"
#include "isp_capability_profile.h"
#include "isp_runtime_intent.h"

namespace ISP
{

/**
 * @brief   : ISP业务服务接口
 * @note    : ipc_share只依赖此接口；业务仓库（hi3516_ipc/rv1126b_ipc）在初始化时实现并注册服务。
 */
class IIspBusinessService
{
public:
    virtual ~IIspBusinessService()
    {
    }

    /**
     * @brief   : 初始化业务侧 ISP 服务
     * @return  : OK：成功，非OK：失败
     */
    virtual int init() = 0;

    /**
     * @brief   : 停止业务侧 ISP 服务
     * @return  : OK：成功，非OK：失败
     */
    virtual int deinit() = 0;

    /**
     * @brief   : 应用单类基础参数
     * @param    {PicConfigureType_E} enType：参数类型
     * @return   {int} OK：成功，非OK：失败
     */
    virtual int update_param(PicConfigureType_E enType) = 0;

    /**
     * @brief   : 日夜配置更新
     * @param    {DayNightAttr_S} stOld：旧配置
     * @param    {DayNightAttr_S} stNew：新配置
     * @return   {int} OK：成功，非OK：失败
     */
    virtual int update_daynight(const DayNightAttr_S &stOld, const DayNightAttr_S &stNew) = 0;

    /**
     * @brief   : 应用网页配置场景及其全部参数
     * @param    {SceneType_E} enScene：目标网页配置场景
     * @return   {int} OK：成功，非OK：失败
     */
    virtual int apply_scene(SceneType_E enScene) = 0;

    /**
     * @brief   : 场景计划变化通知
     * @return  : OK：成功，非OK：失败
     */
    virtual int on_schedule_changed() = 0;

    /**
     * @brief   : 校验图像参数并修正到平台支持范围
     * @param    {ImageParam_S} stConfig：入出参
     * @return   {int} OK：成功，非OK：失败
     */
    virtual int validate_image_param(ImageParam_S &stConfig) = 0;

    /**
     * @brief   : 校验日夜和补光参数并修正到平台支持范围
     * @param    {DayNightAttr_S} stConfig：入出参
     * @return   {int} OK：成功，非OK：失败
     */
    virtual int validate_daynight(DayNightAttr_S &stConfig) = 0;

    /**
     * @brief   : 获取平台支持的功能和参数范围副本
     * @param    {IspCapabilityProfile_S&} stProfile：平台功能和参数范围输出
     * @return   {int} OK：成功，非OK：失败
     * @note    : 新调用路径通过此方法获取平台能力，禁止直接访问平台服务成员。
     */
    virtual int get_capability_profile(IspCapabilityProfile_S &stProfile) const = 0;

    /**
     * @brief   : 按配置值类型应用配置（不保存）
     * @param    {const IspConfigValue_T&} stConfig：配置值
     * @return   {int} OK：成功，非OK：失败
     * @note    : 配置保存由 CIspConfigCommandService 负责；本方法只提交参数或硬件设置。
     */
    virtual int apply_config(const IspConfigValue_T &stConfig) = 0;

    /**
     * @brief   : 按保存的配置重新应用全部硬件设置
     * @return   {int} OK：成功，非OK：失败
     * @note    : 用于启动、恢复默认和错误恢复
     */
    virtual int reconcile_all() = 0;

    /**
     * @brief   : 开始临时优先使用灯光
     * @param    {const IspLightOverride_S&} stOverride：临时灯光设置
     * @param    {uint64_t&} u64Token：输出申请编号
     * @return   {int} OK：成功
     */
    virtual int begin_light_override(const IspLightOverride_S &stOverride, uint64_t &u64Token) = 0;

    /**
     * @brief   : 结束临时优先使用灯光
     * @param    {uint64_t} u64Token：申请时返回的编号
     * @return   {int} OK：成功
     */
    virtual int end_light_override(uint64_t u64Token) = 0;
};

} // namespace ISP
